// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"strings"

	"github.com/husarnet/husarnet/cli/v2/constants"
	"github.com/husarnet/husarnet/cli/v2/types"

	"github.com/mattn/go-runewidth"
	"github.com/pterm/pterm"
	"github.com/urfave/cli/v3"
)

func PrintStatus(cmd *cli.Command, status types.DaemonStatus) {
	isHealthy := status.LiveData.Health.Summary
	verbose := verboseLogs || cmd.Bool("verbose")

	if !isHealthy {
		if !verbose {
			pterm.Println("Note: --verbose flag enabled by default as the daemon state is unhealthy")
			pterm.Println()
		}
		verbose = true
	}

	if verbose {
		printVerboseStatus(cmd, status)
	}

	// section DASHBOARD
	dashboardUrl := fmt.Sprintf("https://dashboard.%s", status.Config.Env.InstanceFqdn)

	printStatusHeader("Dashboard")
	if status.Config.Dashboard.IsClaimed {
		printStatusLine(greenFormatter, "URL", dashboardUrl)
		printStatusLine(neutralFormatter, "Claimed by", status.Config.Dashboard.ClaimInfo.Owner)
		printStatusLine(
			neutralFormatter,
			"Device known as",
			formatHostnameWithAliases(status.Config.Dashboard.ClaimInfo.Hostname, status.Config.Dashboard.ClaimInfo.Aliases),
		)
		// TODO later: add information about the group(s) the device is in, currently api does not expose it
	} else {
		printStatusLine(redFormatter, "URL", dashboardUrl)
		printStatusLine(neutralFormatter, "Claimed by", "none")
	}
	pterm.Println()

	// section THIS DEVICE
	printStatusHeader("This device")
	printStatusLine(neutralFormatter, "IP", pterm.Bold.Sprint(status.LiveData.LocalIP.StringExpanded()))
	printStatusLine(neutralFormatter, "User agent", status.UserAgent)
	if status.LiveData.Health.Summary {
		printStatusLine(greenFormatter, "Health", "healthy")
	} else {
		printStatusLine(redFormatter, "Health", "unhealthy")
	}
	pterm.Println()

	// section PEERS
	peerMap := mapifyPeers(status.LiveData.Peers)
	printStatusHeader("Peers")
	if len(status.Config.Dashboard.Peers) == 0 {
		printIndentedL1("No peers yet")
	} else {
		for _, peer := range status.Config.Dashboard.Peers {
			firstLine := formatPeerInfo(peer.Address.StringExpanded(), peerMap)
			secondLine := "Hostnames: " + formatHostnameWithAliases(peer.Hostname, peer.Aliases)
			printIndentedL1(firstLine)
			printIndentedL1(secondLine)
			pterm.Println()
		}
	}

	// section LOCAL WHITELIST (only visible if not empty)
	if len(status.Config.User.Whitelist) > 0 {
		printWhitelist(status, peerMap)
	}
}

func printVerboseStatus(cmd *cli.Command, status types.DaemonStatus) {
	printStatusHeader("Version")
	printVersion(status.Version)
	pterm.Println()

	printStatusHeader("Healthchecks")
	printIndentedL1("Base servers")
	for _, server := range status.Config.License.BaseServers {
		currentOrBackup := "backup"
		if status.LiveData.BaseConnection.Address == server {
			currentOrBackup = "current"
		}
		printIndentedL2(pterm.Sprintf("%s, %s, %s", server.String(), status.LiveData.BaseConnection.Type, currentOrBackup))
	}

	printIndentedL1("API servers")
	for _, server := range status.Config.License.ApiServers {
		currentOrBackup := "current"
		printIndentedL2(pterm.Sprintf("%s healthy, %s", server.StringExpanded(), currentOrBackup))
	}

	printIndentedL1("Event Bus")
	for _, server := range status.Config.License.EbServers {
		currentOrBackup := "current"
		printIndentedL2(pterm.Sprintf("%s healthy, %s", server.StringExpanded(), currentOrBackup))
	}
	pterm.Println()

	printStatusHeader("Local flags")
	printHooksStatus(status)
	pterm.Println()

	printStatusHeader("Remote flags")
	printStatusLine(neutralFormatter, "AccountAdmin", enabledOrDisabled(status.Config.Dashboard.Features.AccountAdmin))
	printStatusLine(neutralFormatter, "SyncHostname", enabledOrDisabled(status.Config.Dashboard.Features.SyncHostname))
	pterm.Println()
}

func enabledOrDisabled(f bool) string {
	if f {
		return "enabled"
	}
	return "disabled"
}

func printIndentedL1(line string) {
	pterm.Printfln("  %s", line)
}

func printIndentedL2(line string) {
	pterm.Printfln("    %s", line)
}

func mapifyPeers(peers []types.DaemonPeerInfo) map[string]types.DaemonPeerInfo {
	out := make(map[string]types.DaemonPeerInfo)
	for _, peer := range peers {
		out[peer.Address.StringExpanded()] = peer
	}
	return out
}

func printStatusHeader(text string) {
	pterm.Println(text)
}

func printStatusLine(formatter FormatterFunc, name, value string) {
	fill := ""
	desiredLength := 25

	nameLength := runewidth.StringWidth(pterm.RemoveColorFromString(name))
	if nameLength < desiredLength {
		fill = strings.Repeat(" ", desiredLength-nameLength)
	}

	merger := ":"
	if len(name) > 0 && strings.HasSuffix(name, "?") {
		merger = " "
	}

	pterm.Printfln("  %s%s%s %s", name, merger, fill, formatter(value))
}

func printVersion(daemonVersion string) {
	latestVersion := "" // TODO long term - get this from the announcements info

	var versionFormatter FormatterFunc
	var versionHelp string

	if daemonVersion != constants.Version {
		versionFormatter = redFormatter
		versionHelp = "CLI and Husarnet Daemon versions differ. If you updated recently, restart the Daemon"
	} else if daemonVersion != getDaemonBinaryVersion() {
		versionFormatter = yellowFormatter
		versionHelp = "Husarnet Daemon you're running and the one saved on a disk differ"
	} else if latestVersion != "" && constants.Version != latestVersion {
		versionFormatter = yellowFormatter
		versionHelp = "You're not running the latest version of Husarnet"
	} else {
		versionFormatter = greenFormatter
	}

	printStatusLine(versionFormatter, "CLI", constants.Version)
	printStatusLine(versionFormatter, "Daemon (running)", daemonVersion)
	printStatusLine(versionFormatter, "Daemon (binary)", getDaemonBinaryVersion())
	if latestVersion != "" {
		printStatusLine(versionFormatter, "Latest", latestVersion)
	}
	if versionHelp != "" {
		printIndentedL1(versionFormatter(versionHelp))
	}
}

func formatPeerInfo(ip string, peerMap map[string]types.DaemonPeerInfo) string {
	line := ip
	if info, ok := peerMap[ip]; ok {
		activity := "inactive"
		if info.IsActive {
			activity = "active"
		}
		connection := greenFormatter("peer to peer")
		if info.IsTunelled {
			connection = yellowFormatter("tunelled")
		}
		line += " " + activity + ", " + connection
	} else {
		line += " unknown"
	}
	return line
}

func formatHostnameWithAliases(hostname string, aliases []string) string {
	var out string
	if len(aliases) > 0 {
		out += pterm.Bold.Sprint(hostname)
		out += ", "
		out += strings.Join(aliases, ", ")
	} else {
		out += hostname
	}
	return out
}

func printWhitelist(status types.DaemonStatus, peerMap map[string]types.DaemonPeerInfo) {
	printStatusHeader("Local whitelist")
	for _, peerAddr := range status.Config.User.Whitelist {
		printIndentedL1(formatPeerInfo(peerAddr.StringExpanded(), peerMap))
	}
}

func printHooksStatus(status types.DaemonStatus) {
	var formatter FormatterFunc
	var value, help string

	if status.Config.Env.HooksEnabled {
		formatter = greenFormatter
		value = "enabled"
	} else {
		formatter = neutralFormatter
		value = "disabled"
	}

	printStatusLine(formatter, "Hooks", value)
	if help != "" {
		printIndentedL1(help)
	}
}
