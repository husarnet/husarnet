// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"github.com/husarnet/husarnet/cli/v2/config"
	"net/netip"
	"sort"
	"strings"
	"time"

	"github.com/gobeam/stringy"
	"github.com/mattn/go-runewidth"
	"github.com/pterm/pterm"
	u "github.com/rjNemo/underscore"
	"github.com/urfave/cli/v3"
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

	merger := ":"
	if len(name) > 0 && strings.HasSuffix(name, "?") {
		merger = " "
	}

	pterm.Printfln("%s %s%s%s %s", dot, name, merger, fill, formatter(value))
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
	printBoolStatus(status, name, "yes", "no")
}

func printVersion(daemonVersion string) {
	latestVersion := "" // TODO long term - get this from the announcements info

	var versionDot, versionHelp string

	if daemonVersion != cliVersion {
		versionDot = redDot
		versionHelp = "CLI and Husarnet Daemon versions differ! If you updated recently, restart the Daemon"
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
			pterm.Printf("%s secure       ", greenDot)
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

func printHooksStatus(status DaemonStatus) {
	var dot, value, help string

	if status.HooksEnabled {
		dot = greenDot
		value = "enabled"
	} else {
		dot = yellowDot
		value = "disabled"
		help = "Disabled hooks are not a reason to panic, unless your setup needs them"
	}

	printStatusLine(dot, "Hooks", value)
	if help != "" {
		printStatusHelp(dot, help)
	}
}

func printStatus(cmd *cli.Command, status DaemonStatus) {
	verbose := verboseLogs || cmd.Bool("verbose")

	printStatusHeader("Version")
	printVersion(status.Version)
	pterm.Println()

	printStatusHeader("Feature flags")
	printHooksStatus(status)
	pterm.Println()

	var dashboardDot, dashboardHelp string
	if status.DashboardFQDN != config.GetDefaultDashboardUrl() {
		dashboardDot = yellowDot
		dashboardHelp = "You're using custom / self-hosted environment"
	} else {
		dashboardDot = greenDot
	}

	printStatusHeader("Dashboard")
	printStatusLine(dashboardDot, "URL", status.DashboardFQDN)
	if dashboardHelp != "" {
		printStatusHelp(dashboardDot, dashboardHelp)
	}
	handleStandardResult(status.StdResult)
	pterm.Println()

	var baseServerDot, baseServerHelp, baseServerStatusReminder string

	if status.BaseConnection.Type == "UDP" {
		baseServerDot = greenDot
	} else if status.BaseConnection.Type == "TCP" {
		baseServerDot = yellowDot
		baseServerHelp = "TCP is a fallback connection method. You'll get better results on UDP"
		baseServerStatusReminder = "You can check on https://status.husarnet.com if there are any issues with our infrastructure"
	} else {
		baseServerDot = redDot
		baseServerHelp = "There's no Base Server connection - Husarnet will not be fully functional"
		baseServerStatusReminder = "You can check on https://status.husarnet.com if there are any issues with our infrastructure"
	}

	printStatusHeader("Connection status")
	printStatusLine(baseServerDot, "Base Server", pterm.Sprintf("%s:%v (%s)", status.BaseConnection.Address, status.BaseConnection.Port, status.BaseConnection.Type))
	if baseServerHelp != "" {
		printStatusHelp(baseServerDot, baseServerHelp)
	}
	if baseServerStatusReminder != "" {
		printStatusHelp(baseServerDot, baseServerStatusReminder)
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

func printStatusFollow(cmd *cli.Command) {
	// Obtaining status twice is simpler than deep copy
	prevStatus := getDaemonStatus()
	prevStatus.Version = "temporary change to enable print"
	var currStatus DaemonStatus
	for {
		currStatus = getDaemonStatus()
		if !areStatusesEqual(prevStatus, currStatus) {
			printStatus(cmd, currStatus)
			prevStatus = currStatus
		}
		time.Sleep(500 * time.Millisecond)
	}
}

func areStatusesEqual(prevStatus, currStatus DaemonStatus) bool {
	if prevStatus.Version != currStatus.Version {
		return false
	}
	if prevStatus.DashboardFQDN != currStatus.DashboardFQDN {
		return false
	}
	if prevStatus.WebsetupAddress.Compare(currStatus.WebsetupAddress) != 0 {
		return false
	}
	if prevStatus.BaseConnection.Address.Compare(currStatus.BaseConnection.Address) != 0 {
		return false
	}
	if prevStatus.BaseConnection.Port != currStatus.BaseConnection.Port {
		return false
	}
	if prevStatus.BaseConnection.Type != currStatus.BaseConnection.Type {
		return false
	}
	if prevStatus.LocalIP.Compare(currStatus.LocalIP) != 0 {
		return false
	}
	if prevStatus.LocalHostname != currStatus.LocalHostname {
		return false
	}
	if prevStatus.IsJoined != currStatus.IsJoined {
		return false
	}
	if prevStatus.IsReady != currStatus.IsReady {
		return false
	}
	if prevStatus.IsReadyToJoin != currStatus.IsReadyToJoin {
		return false
	}
	if !areStandardResultsEqual(prevStatus.StdResult, currStatus.StdResult) {
		return false
	}
	if !areConnectionStatusesEqual(prevStatus.ConnectionStatus, currStatus.ConnectionStatus) {
		return false
	}
	if !areAddressListsEqual(prevStatus.Whitelist, currStatus.Whitelist) {
		return false
	}
	if !areUserSettingsEqual(prevStatus.UserSettings, currStatus.UserSettings) {
		return false
	}
	if !areHostTablesEqual(prevStatus.HostTable, currStatus.HostTable) {
		return false
	}
	if !arePeersEqual(prevStatus.Peers, currStatus.Peers) {
		return false
	}
	return true

}

func areStandardResultsEqual(a, b StandardResult) bool {
	if a.IsDirty != b.IsDirty {
		return false
	}
	return true
}

func areConnectionStatusesEqual(a, b map[string]bool) bool {
	if len(a) != len(b) {
		return false
	}
	for key, value := range a {
		if value != b[key] {
			return false
		}
	}
	return true
}

func areAddressListsEqual(a, b []netip.Addr) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if a[i].Compare(b[i]) != 0 {
			return false
		}
	}
	return true
}

func arePortAddressListsEqual(a, b []netip.AddrPort) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if (a[i].Addr().Compare(b[i].Addr()) != 0) || (a[i].Port() != b[i].Port()) {
			return false
		}
	}
	return true
}

func areUserSettingsEqual(a, b map[string]string) bool {
	if len(a) != len(b) {
		return false
	}
	for key, value := range a {
		if value != b[key] {
			return false
		}
	}
	return true
}

func areHostTablesEqual(a, b map[string]netip.Addr) bool {
	if len(a) != len(b) {
		return false
	}
	for key, value := range a {
		if value.Compare(b[key]) != 0 {
			return false
		}
	}
	return true
}

func arePeerStatusesEqual(a, b PeerStatus) bool {
	if a.HusarnetAddress.Compare(b.HusarnetAddress) != 0 {
		return false
	}
	if (a.LinkLocalAddress.Addr().Compare(b.LinkLocalAddress.Addr()) != 0) || (a.LinkLocalAddress.Port() != b.LinkLocalAddress.Port()) {
		return false
	}
	if a.IsActive != b.IsActive {
		return false
	}
	if a.IsReestablishing != b.IsReestablishing {
		return false
	}
	if a.IsSecure != b.IsSecure {
		return false
	}
	if a.IsTunelled != b.IsTunelled {
		return false
	}
	if !arePortAddressListsEqual(a.SourceAddresses, b.SourceAddresses) {
		return false
	}
	if !arePortAddressListsEqual(a.TargetAddresses, b.TargetAddresses) {
		return false
	}
	if (a.UsedTargetAddress.Addr().Compare(b.UsedTargetAddress.Addr()) != 0) || (a.UsedTargetAddress.Port() != b.UsedTargetAddress.Port()) {
		return false
	}
	return true
}

func arePeersEqual(a, b []PeerStatus) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if !arePeerStatusesEqual(a[i], b[i]) {
			return false
		}
	}
	return true
}
