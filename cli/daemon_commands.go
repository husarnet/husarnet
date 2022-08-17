// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"net/url"
	"sort"
	"strings"
	"time"

	"github.com/pterm/pterm"
	"github.com/urfave/cli/v2"
	"golang.org/x/text/cases"
	"golang.org/x/text/language"
)

var daemonStatusCommand = &cli.Command{
	Name:  "status",
	Usage: "Display current connectivity status",
	Action: func(ctx *cli.Context) error {
		status := getDaemonStatus()

		errorStyle := pterm.NewStyle(pterm.FgWhite, pterm.BgRed)
		warningStyle := pterm.NewStyle(pterm.FgBlack, pterm.BgYellow)

		casualStyle := pterm.NewStyle()

		flashyStyle := pterm.NewStyle(pterm.Bold)
		greenStyle := pterm.NewStyle(pterm.Bold, pterm.BgGreen, pterm.FgWhite)

		versionStyle := casualStyle
		dashboardStyle := casualStyle
		baseConnectionStyle := casualStyle

		if version != status.Version {
			versionStyle = errorStyle
		}

		if husarnetDashboardFQDN != defaultDashboard {
			dashboardStyle = warningStyle
		}

		if husarnetDashboardFQDN != status.DashboardFQDN {
			dashboardStyle = errorStyle
		}

		statusItems := []pterm.BulletListItem{
			{Level: 0, Text: "Version", TextStyle: versionStyle},
			{Level: 1, Text: fmt.Sprintf("Daemon: %s", status.Version), TextStyle: versionStyle},
			{Level: 1, Text: fmt.Sprintf("CLI:    %s", version), TextStyle: versionStyle},
			{Level: 0, Text: "Dashboard", TextStyle: dashboardStyle},
			{Level: 1, Text: fmt.Sprintf("Daemon: %s", status.DashboardFQDN), TextStyle: dashboardStyle},
			{Level: 1, Text: fmt.Sprintf("CLI:    %s", husarnetDashboardFQDN), TextStyle: dashboardStyle},
			{Level: 0, Text: "Husarnet address"},
			{Level: 1, Text: status.LocalIP.String(), Bullet: " ", TextStyle: greenStyle}, // This is on a separate line and without a bullet so it can be easily copied with a triple click
			{Level: 0, Text: fmt.Sprintf("Local hostname: %s", status.LocalHostname)},
			{Level: 0, Text: "Whitelist"},
		}

		sort.Slice(status.Whitelist, func(i, j int) bool { return status.Whitelist[i].String() < status.Whitelist[j].String() })

		for _, address := range status.Whitelist {
			statusItems = append(statusItems, pterm.BulletListItem{Level: 1, Text: address.String(), TextStyle: flashyStyle})

			var peerHostnames []string

			for hostname, HTaddress := range status.HostTable {
				if address == HTaddress {
					peerHostnames = append(peerHostnames, hostname)
				}
			}

			if len(peerHostnames) > 0 {
				sort.Strings(peerHostnames)
				statusItems = append(statusItems, pterm.BulletListItem{Level: 2, Text: fmt.Sprintf("Hostnames: %s", pterm.Bold.Sprintf(strings.Join(peerHostnames, ", ")))})
			}

			if address == status.WebsetupAddress {
				statusItems = append(statusItems, pterm.BulletListItem{Level: 2, Text: pterm.Bold.Sprint("Role: websetup")})
			}

			for _, peer := range status.Peers {
				if peer.HusarnetAddress != address {
					continue
				}

				tags := []string{}

				if peer.IsActive {
					tags = append(tags, "active")
				}
				if peer.IsTunelled {
					tags = append(tags, "tunelled")
				}
				if peer.IsSecure {
					tags = append(tags, "secure")
				}

				statusItems = append(statusItems, pterm.BulletListItem{Level: 2, Text: fmt.Sprintf("Tags: %v", strings.Join(tags, ", "))})

				// TODO reenable this after latency support is readded
				// if peer.LatencyMs != -1 {
				// 	statusItems = append(statusItems, pterm.BulletListItem{Level: 2, Text: fmt.Sprintf("Latency (ms): %v", peer.LatencyMs)})
				// }

				if !peer.UsedTargetAddress.Addr().IsUnspecified() {
					statusItems = append(statusItems, pterm.BulletListItem{Level: 2, Text: fmt.Sprintf("Used regular network address: %v", peer.UsedTargetAddress)})
				}

				if verboseLogs {
					statusItems = append(statusItems, pterm.BulletListItem{Level: 2, Text: fmt.Sprintf("Advertised network addresses: %v", peer.TargetAddresses)})
				}

			}
		}

		statusItems = append(statusItems, pterm.BulletListItem{Level: 0, Text: "Connection status"})

		// As UDP is the desired one - warn wbout everything that's not it
		if status.BaseConnection.Type == "UDP" {
			baseConnectionStyle = casualStyle
		} else if status.BaseConnection.Type == "TCP" {
			baseConnectionStyle = warningStyle
		} else {
			baseConnectionStyle = errorStyle
		}
		statusItems = append(statusItems, pterm.BulletListItem{Level: 1, Text: fmt.Sprintf("Base server: %s:%v (%s)", status.BaseConnection.Address, status.BaseConnection.Port, status.BaseConnection.Type), TextStyle: baseConnectionStyle})

		if verboseLogs {
			for element, connectionStatus := range status.ConnectionStatus {
				statusItems = append(statusItems, pterm.BulletListItem{Level: 1, Text: fmt.Sprintf("%s: %t", cases.Title(language.English).String(element), connectionStatus)})
			}
		}

		statusItems = append(statusItems, pterm.BulletListItem{Level: 0, Text: "Readiness"})
		statusItems = append(statusItems, pterm.BulletListItem{Level: 1, Text: fmt.Sprintf("Is ready to handle data: %v", status.IsReady)})
		if status.IsJoined {
			statusItems = append(statusItems, pterm.BulletListItem{Level: 1, Text: fmt.Sprintf("Is joined/adopted: %v", status.IsJoined)})
		} else {
			statusItems = append(statusItems, pterm.BulletListItem{Level: 1, Text: fmt.Sprintf("Is ready to be joined/adopted: %v", status.IsReadyToJoin)})
		}

		if verboseLogs {
			statusItems = append(statusItems, pterm.BulletListItem{Level: 0, Text: "User settings"})
			for settingName, settingValue := range status.UserSettings {
				statusItems = append(statusItems, pterm.BulletListItem{Level: 1, Text: fmt.Sprintf("%s: %v", settingName, settingValue)})
			}
		}

		pterm.DefaultBulletList.WithItems(statusItems).Render()

		return nil
	},
}

var daemonJoinCommand = &cli.Command{
	Name:  "join",
	Usage: "Connect to Husarnet group with given join code and with specified hostname",
	Action: func(ctx *cli.Context) error {
		// up to two params
		if ctx.Args().Len() < 1 {
			die("you need to provide joincode")
		}
		joincode := ctx.Args().Get(0)
		hostname := ""

		if ctx.Args().Len() == 2 {
			hostname = ctx.Args().Get(1)
		}

		params := url.Values{
			"code":     {joincode},
			"hostname": {hostname},
		}
		addDaemonApiSecret(&params)
		callDaemonPost[EmptyResult]("/control/join", params)
		return nil
	},
}

var daemonSetupServerCommand = &cli.Command{
	Name:  "setup-server",
	Usage: "Connect your Husarnet device to different Husarnet infrastructure",
	Action: func(ctx *cli.Context) error {
		if ctx.Args().Len() < 1 {
			fmt.Println("you need to provide address of the dashboard")
			return nil
		}

		domain := ctx.Args().Get(0)

		params := url.Values{
			"domain": {domain},
		}
		addDaemonApiSecret(&params)
		callDaemonPost[EmptyResult]("/control/change-server", params)
		return nil
	},
}

var daemonWhitelistCommand = &cli.Command{
	Name:  "whitelist",
	Usage: "Manage whitelist on the device.",
	Subcommands: []*cli.Command{
		{
			Name:  "ls",
			Usage: "list entries on the whitelist",
			Action: func(ctx *cli.Context) error {
				// callDaemonGet("/control/whitelist/ls") TODO
				return nil
			},
		},
		{
			Name:  "add",
			Usage: "add a device to your whitelist by Husarnet address",
			Action: func(ctx *cli.Context) error {
				if ctx.Args().Len() < 1 {
					fmt.Println("you need to provide Husarnet address of the device")
					return nil
				}
				addr := ctx.Args().Get(0)

				params := url.Values{
					"address": {addr},
				}
				addDaemonApiSecret(&params)
				callDaemonPost[EmptyResult]("/control/whitelist/add", params)
				return nil
			},
		},
		{
			Name:  "rm",
			Usage: "remove device from the whitelist",
			Action: func(ctx *cli.Context) error {
				if ctx.Args().Len() < 1 {
					fmt.Println("you need to provide Husarnet address of the device")
					return nil
				}
				addr := ctx.Args().Get(0)

				params := url.Values{
					"address": {addr},
				}
				addDaemonApiSecret(&params)
				callDaemonPost[EmptyResult]("/control/whitelist/rm", params)
				return nil
			},
		},
	},
}

func waitAction(ctx *cli.Context, message string, lambda func(*DaemonStatus) bool) error {
	if ctx.Args().Len() > 0 {
		cli.ShowSubcommandHelp(ctx)
	}

	spinner, _ := pterm.DefaultSpinner.Start(message)

	failedCounter := 0
	for {
		// TODO don't crash the wait command if the deamon is not yet responding
		status := getDaemonStatus()
		success := lambda(&status)
		time.Sleep(time.Second)
		if success {
			spinner.Success()
			return nil
		}
		failedCounter++
		if failedCounter > 60 {
			spinner.Fail()
			return fmt.Errorf("timeout while waiting for a given service")
		}
	}
}

func waitBaseANY(ctx *cli.Context) error {
	return waitAction(ctx, "Waiting for Base server connection (any protocol)…", func(status *DaemonStatus) bool {
		return status.BaseConnection.Type == "TCP" || status.BaseConnection.Type == "UDP"
	})
}

func waitBaseUDP(ctx *cli.Context) error {
	return waitAction(ctx, "Waiting for Base server connection (UDP)…", func(status *DaemonStatus) bool { return status.BaseConnection.Type == "UDP" })
}

func waitWebsetup(ctx *cli.Context) error {
	return waitAction(ctx, "Waiting for websetup connection…", func(status *DaemonStatus) bool { return status.ConnectionStatus["websetup"] })
}

var daemonWaitCommand = &cli.Command{
	Name:  "wait",
	Usage: "Wait until certain events occur. If no events provided will wait for as many elements as it can (the best case scenario). Husarnet will continue working even if some of those elements are unreachable, so consider narrowing your search down a bit.",
	Action: func(ctx *cli.Context) error {
		waitBaseANY(ctx)
		waitBaseUDP(ctx)
		waitWebsetup(ctx)
		return nil
	},
	Subcommands: []*cli.Command{
		{
			Name:  "base",
			Usage: "Wait until there is a base-server connection established (via any protocol)",
			Action: func(ctx *cli.Context) error {
				waitBaseANY(ctx)
				return nil
			},
			Subcommands: []*cli.Command{
				{
					Name:  "udp",
					Usage: "Wait until there is a base-server connection established via UDP. This is the best case scenario. Husarnet will work even without it.",
					Action: func(ctx *cli.Context) error {
						waitBaseUDP(ctx)
						return nil
					},
				},
			},
		},
		{
			Name:  "join",
			Usage: "Wait until there is enough connectivity to join a network/adopt a device",
			Action: func(ctx *cli.Context) error {
				waitWebsetup(ctx)
				return nil
			},
		},
	},
}

var daemonCommand = &cli.Command{
	Name:  "daemon",
	Usage: "Control the local daemon",
	Subcommands: []*cli.Command{
		daemonStatusCommand,
		daemonJoinCommand,
		daemonSetupServerCommand,
		daemonWhitelistCommand,
		daemonWaitCommand,
	},
}
