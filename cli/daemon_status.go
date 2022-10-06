// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"net/netip"
	"sort"
	"strings"

	"github.com/gobeam/stringy"
	"github.com/mattn/go-runewidth"
	"github.com/pterm/pterm"
	u "github.com/rjNemo/underscore"
	"github.com/urfave/cli/v2"
	"golang.org/x/text/cases"
	"golang.org/x/text/language"
)

func printStatusHeader(text string) {
	pterm.Underscore.Println(text)
}

func printStatusLine(dot, name, value string) {
	formatter := extractFormatter(dot)

	fill := ""
	desiredLength := 25

	nameLength := runewidth.StringWidth(pterm.RemoveColorFromString(name))
	if nameLength < desiredLength {
		fill = strings.Repeat(" ", desiredLength-nameLength)
	}

	pterm.Printfln("%s %s:%s %s", dot, name, fill, formatter(value))
}

func printStatusHelp(dot, help string) {
	formatter := extractFormatter(dot)
	pterm.Printfln("%s %s", dot, formatter(help))
}

func printBoolStatus(status bool, name, trueString, falseString string) {
	var dot, value string

	if status {
		dot = greenDot
		value = trueString
	} else {
		dot = redDot
		value = falseString
	}

	printStatusLine(dot, name, value)
}

func printConnectionStatus(status bool, name string) {
	printBoolStatus(status, name, "connected", "not connected")
}

func printReadinessStatus(status bool, name string) {
	printBoolStatus(status, name, "ready", "not ready")
}

func printVersion(daemonVersion string) {
	latestVersion := "" // TODO long term - get this from the announcements info

	var versionDot, versionHelp string

	if daemonVersion != cliVersion {
		versionDot = redDot
		versionHelp = "CLI and Husarnet Daemon versions differ - you may get unexpected results!"
	} else if daemonVersion != getDaemonBinaryVersion() {
		versionDot = yellowDot
		versionHelp = "Husarnet Daemon you're running and the one saved on a disk differ!"
	} else if latestVersion != "" && cliVersion != latestVersion {
		versionDot = yellowDot
		versionHelp = "You're not running the latest version of Husarnet"
	} else {
		versionDot = greenDot
	}

	printStatusLine(versionDot, "CLI", cliVersion)
	printStatusLine(versionDot, "Daemon (running)", daemonVersion)
	printStatusLine(versionDot, "Daemon (binary)", getDaemonBinaryVersion())
	if latestVersion != "" {
		printStatusLine(versionDot, "Latest", latestVersion)
	}
	if versionHelp != "" {
		printStatusHelp(versionDot, versionHelp)
	}
}

func printWhitelist(status DaemonStatus, verbose bool) {
	sort.Slice(status.Whitelist, func(i, j int) bool { return status.Whitelist[i].String() < status.Whitelist[j].String() })

	for _, peerAddress := range status.Whitelist {
		var peerHostnames []string
		for hostname, hostTableAddress := range status.HostTable {
			if peerAddress == hostTableAddress {
				peerHostnames = append(peerHostnames, hostname)
			}
		}

		var peerData PeerStatus
		for _, peer := range status.Peers {
			if peer.HusarnetAddress != peerAddress {
				continue
			}
			peerData = peer
		}

		sort.Strings(peerHostnames)

		if peerAddress == status.WebsetupAddress {
			peerHostnames = append([]string{"(websetup)"}, peerHostnames...)
		}

		if peerAddress == status.LocalIP {
			peerHostnames = append([]string{"(localhost)"}, peerHostnames...)
		}

		pterm.Printfln("%s %s", peerAddress.StringExpanded(), strings.Join(peerHostnames, " "))

		if peerData.IsActive {
			pterm.Printf("%s active   ", greenDot)
		} else {
			pterm.Printf("%s inactive ", neutralDot)
		}

		pterm.Print("  ") // Spacing

		if peerData.IsSecure {
			pterm.Printf("%s secure        ", greenDot)
		} else {
			pterm.Printf("%s no data flow ", neutralDot)
		}

		pterm.Print("  ") // Spacing

		if peerData.IsTunelled {
			pterm.Printf("%s tunelled     ", yellowDot)
		} else {
			pterm.Printf("%s peer to peer ", greenDot)
		}

		pterm.Println()

		if verbose {
			if len(peerData.TargetAddresses) > 0 {
				targetAddresses := u.Map(peerData.TargetAddresses, func(it netip.AddrPort) string { return it.String() })
				targetAddresses = removeDuplicates(targetAddresses)
				sort.Strings(targetAddresses)

				pterm.Printfln("Addresses from Base Server: %s", strings.Join(targetAddresses, " "))
			}

			if !peerData.UsedTargetAddress.Addr().IsUnspecified() && peerData.UsedTargetAddress.Addr() != status.BaseConnection.Address {
				pterm.Printfln("Used destination address:   %v", peerData.UsedTargetAddress)
			}
		}

		pterm.Println()
	}
}

func printStatus(ctx *cli.Context) {
	verbose := verboseLogs || ctx.Bool("verbose")

	status := getDaemonStatus()

	printStatusHeader("Version")
	printVersion(status.Version)
	pterm.Println()

	var dashboardDot, dashboardHelp string

	if status.DashboardFQDN != husarnetDashboardFQDN {
		dashboardDot = redDot
		dashboardHelp = "Dashboards used by the CLI and Husarnet Daemon differ - you may get unexpected results!"
	} else if husarnetDashboardFQDN != defaultDashboard && strings.HasSuffix(husarnetDashboardFQDN, ".husarnet.com") {
		dashboardDot = yellowDot
		dashboardHelp = "You're using development environment"
	} else if husarnetDashboardFQDN != defaultDashboard {
		dashboardDot = yellowDot
		dashboardHelp = "You're using self-hosted environment"
	} else {
		dashboardDot = greenDot
	}

	printStatusHeader("Dashboard URL")
	printStatusLine(dashboardDot, "CLI", husarnetDashboardFQDN)
	printStatusLine(dashboardDot, "Daemon", status.DashboardFQDN)
	if dashboardHelp != "" {
		printStatusHelp(dashboardDot, dashboardHelp)
	}
	if status.IsDirty {
		printStatusHelp(redDot, "Daemon's dirty flag is set. You need to restart husarnet-daemon in order to reflect the current settings (like the Dashboard URL)")
	}
	pterm.Println()

	var baseServerDot, baseServerHelp string

	if status.BaseConnection.Type == "UDP" {
		baseServerDot = greenDot
	} else if status.BaseConnection.Type == "TCP" {
		baseServerDot = yellowDot
		baseServerHelp = "TCP is a fallback connection method. You'll get better results on UDP"
	} else {
		baseServerDot = redDot
		baseServerHelp = "There's no Base Server connection - Husarnet will not be fully functional"
	}

	printStatusHeader("Connection status")
	printStatusLine(baseServerDot, "Base Server", pterm.Sprintf("%s:%v (%s)", status.BaseConnection.Address, status.BaseConnection.Port, status.BaseConnection.Type))
	if baseServerHelp != "" {
		printStatusHelp(baseServerDot, baseServerHelp)
	}

	if verbose {
		for element, connectionStatus := range status.ConnectionStatus {
			if element == "base" {
				continue // We already covered base server in detail
			}

			printConnectionStatus(connectionStatus, cases.Title(language.English).String(element))
		}
	}
	pterm.Println()

	printStatusHeader("Readiness")
	printReadinessStatus(status.IsReady, "Is ready to handle data?")
	printReadinessStatus(status.IsReadyToJoin, "Is ready to join?")
	printReadinessStatus(status.IsJoined, "Is joined?")
	pterm.Println()

	if verbose {
		printStatusHeader("User settings")

		settingKeys := []string{}
		for k := range status.UserSettings {
			settingKeys = append(settingKeys, k)
		}
		sort.Strings(settingKeys)

		for _, settingName := range settingKeys {
			settingValue := status.UserSettings[settingName]
			settingName = stringy.New(settingName).SnakeCase().ToUpper()
			printStatusLine(neutralDot, settingName, settingValue)
		}
		pterm.Println()
		pterm.Printfln("If you want to override any of those variables, prefix them with %s and provide as an environment variable to %s", pterm.Bold.Sprint("HUSARNET_"), pterm.Bold.Sprint("husarnet-daemon"))
		pterm.Println()
	}

	printStatusHeader("Local")
	// TODO long term - those two will need to query dashboard, but you still need a status to be near instant so set the timeouts to something line 200ms max
	// printStatusLine(neutralDot, "Hostname", status.LocalHostname, "") TODO long term - get the hostname we're using on the Husarnet network
	// printStatusLine(neutralDot, "Groups", status.LocalHostname, "") TODO long term - query dashboard (thus move this command to the compound one) for the grups current host is in
	pterm.Printfln("%s Husarnet IP:\n%s", neutralDot, pterm.Bold.Sprint(status.LocalIP.StringExpanded()))
	pterm.Println()

	printStatusHeader("Whitelist")
	printWhitelist(status, verbose)

}
