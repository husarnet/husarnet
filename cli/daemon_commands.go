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

func waitAction(message string, lambda func(*DaemonStatus) bool) error {
	spinner, _ := pterm.DefaultSpinner.Start(message)

	failedCounter := 0
	for {
		failedCounter++
		if failedCounter > 60 {
			spinner.Fail()
			return fmt.Errorf("timeout while waiting for a given service")
		}

		response, err := getDaemonStatusRaw(false)
		if err != nil {
			spinner.Fail(err)
			time.Sleep(time.Second)
			continue
		}

		success := lambda(&response.Result)
		if success {
			spinner.Success()
			return nil
		}

		time.Sleep(time.Second)
	}
}

func waitDaemon() error {
	return waitAction("Waiting until we can communicate with husarnet daemon…", func(status *DaemonStatus) bool { return true })
}

func waitBaseANY() error {
	return waitAction("Waiting for Base server connection (any protocol)…", func(status *DaemonStatus) bool {
		return status.BaseConnection.Type == "TCP" || status.BaseConnection.Type == "UDP"
	})
}

func waitBaseUDP() error {
	return waitAction("Waiting for Base server connection (UDP)…", func(status *DaemonStatus) bool { return status.BaseConnection.Type == "UDP" })
}

func waitWebsetup() error {
	return waitAction("Waiting for websetup connection…", func(status *DaemonStatus) bool { return status.ConnectionStatus["websetup"] })
}

func waitJoined() error {
	return waitAction("Waiting until the device is joined…", func(status *DaemonStatus) bool { return status.IsJoined })
}

func makeBullet(level int, text string, textStyle *pterm.Style) pterm.BulletListItem {
	return pterm.BulletListItem{
		Level:       level * 4,
		Bullet:      "»",
		BulletStyle: defaultStyle,
		Text:        text,
		TextStyle:   textStyle,
	}
}

func getWhitelistBullets(status DaemonStatus) []pterm.BulletListItem {
	statusItems := []pterm.BulletListItem{}

	sort.Slice(status.Whitelist, func(i, j int) bool { return status.Whitelist[i].String() < status.Whitelist[j].String() })

	for _, address := range status.Whitelist {
		statusItems = append(statusItems, makeBullet(1, address.String(), flashyStyle))

		var peerHostnames []string

		for hostname, HTaddress := range status.HostTable {
			if address == HTaddress {
				peerHostnames = append(peerHostnames, hostname)
			}
		}

		if len(peerHostnames) > 0 {
			sort.Strings(peerHostnames)
			statusItems = append(statusItems, makeBullet(2, fmt.Sprintf("Hostnames: %s", pterm.Bold.Sprintf(strings.Join(peerHostnames, ", "))), defaultStyle))
		}

		if address == status.WebsetupAddress {
			statusItems = append(statusItems, makeBullet(2, pterm.Bold.Sprint("Role: websetup"), defaultStyle))
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

			statusItems = append(statusItems, makeBullet(2, fmt.Sprintf("Tags: %v", strings.Join(tags, ", ")), defaultStyle))

			// TODO reenable this after latency support is readded
			// if peer.LatencyMs != -1 {
			// 	statusItems = append(statusItems, pterm.BulletListItem{Level: 2, Text: fmt.Sprintf("Latency (ms): %v", peer.LatencyMs)})
			// }

			if !peer.UsedTargetAddress.Addr().IsUnspecified() {
				statusItems = append(statusItems, makeBullet(2, fmt.Sprintf("Used regular network address: %v", peer.UsedTargetAddress), defaultStyle))
			}

			if verboseLogs {
				statusItems = append(statusItems, makeBullet(2, fmt.Sprintf("Advertised network addresses: %v", peer.TargetAddresses), defaultStyle))
			}

		}
	}

	return statusItems
}

var daemonStartCommand = &cli.Command{
	Name:      "start",
	Usage:     "start husarnet daemon",
	ArgsUsage: " ", // No arguments needed
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:        "wait",
			Aliases:     []string{"w"},
			Usage:       "wait until the daemon is running",
			Destination: &wait,
		},
	},
	Action: func(ctx *cli.Context) error {
		runSubcommand(false, "sudo", "systemctl", "start", "husarnet")
		printSuccess("Started husarnet-daemon")

		if wait {
			waitDaemon()
		}

		return nil
	},
}

var daemonRestartCommand = &cli.Command{
	Name:      "restart",
	Usage:     "restart husarnet daemon",
	ArgsUsage: " ", // No arguments needed
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:        "wait",
			Aliases:     []string{"w"},
			Usage:       "wait until the daemon is running",
			Destination: &wait,
		},
	},
	Action: func(ctx *cli.Context) error {
		runSubcommand(false, "sudo", "systemctl", "restart", "husarnet")
		printSuccess("Restarted husarnet-daemon")

		if wait {
			waitDaemon()
		}

		return nil
	},
}

var daemonStopCommand = &cli.Command{
	Name:      "stop",
	Usage:     "stop husarnet daemon",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx *cli.Context) error {
		runSubcommand(false, "sudo", "systemctl", "stop", "husarnet")
		printSuccess("Stopped husarnet-daemon")
		return nil
	},
}

var daemonStatusCommand = &cli.Command{
	Name:      "status",
	Usage:     "Display current connectivity status",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx *cli.Context) error {
		status := getDaemonStatus()

		versionStyle := casualStyle
		dashboardStyle := casualStyle
		var baseConnectionStyle *pterm.Style

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
			makeBullet(0, "Version", versionStyle),
			makeBullet(1, fmt.Sprintf("Daemon: %s", status.Version), versionStyle),
			makeBullet(1, fmt.Sprintf("CLI:    %s", version), versionStyle),
			makeBullet(0, "Dashboard", dashboardStyle),
			makeBullet(1, fmt.Sprintf("Daemon: %s", status.DashboardFQDN), dashboardStyle),
			makeBullet(1, fmt.Sprintf("CLI:    %s", husarnetDashboardFQDN), dashboardStyle),
			makeBullet(0, "Husarnet address", defaultStyle),
			makeBullet(1, status.LocalIP.String(), greenStyle), // This is on a separate line and without a bullet so it can be easily copied with a triple cli)k
			makeBullet(0, fmt.Sprintf("Local hostname: %s", status.LocalHostname), defaultStyle),
			makeBullet(0, "Whitelist", defaultStyle),
		}

		statusItems = append(statusItems, getWhitelistBullets(status)...)

		statusItems = append(statusItems, makeBullet(0, "Connection status", defaultStyle))

		// As UDP is the desired one - warn wbout everything that's not it
		if status.BaseConnection.Type == "UDP" {
			baseConnectionStyle = casualStyle
		} else if status.BaseConnection.Type == "TCP" {
			baseConnectionStyle = warningStyle
		} else {
			baseConnectionStyle = errorStyle
		}
		statusItems = append(statusItems, makeBullet(1, fmt.Sprintf("Base server: %s:%v (%s)", status.BaseConnection.Address, status.BaseConnection.Port, status.BaseConnection.Type), baseConnectionStyle))

		if verboseLogs {
			for element, connectionStatus := range status.ConnectionStatus {
				statusItems = append(statusItems, makeBullet(1, fmt.Sprintf("%s: %t", cases.Title(language.English).String(element), connectionStatus), defaultStyle))
			}
		}

		statusItems = append(statusItems, makeBullet(0, "Readiness", defaultStyle))
		statusItems = append(statusItems, makeBullet(1, fmt.Sprintf("Is ready to handle data: %v", status.IsReady), defaultStyle))
		if status.IsJoined {
			statusItems = append(statusItems, makeBullet(1, fmt.Sprintf("Is joined/adopted: %v", status.IsJoined), defaultStyle))
		} else {
			statusItems = append(statusItems, makeBullet(1, fmt.Sprintf("Is ready to be joined/adopted: %v", status.IsReadyToJoin), defaultStyle))
		}

		if verboseLogs {
			statusItems = append(statusItems, makeBullet(0, "User settings", defaultStyle))
			for settingName, settingValue := range status.UserSettings {
				statusItems = append(statusItems, makeBullet(1, fmt.Sprintf("%s: %v", settingName, settingValue), defaultStyle))
			}
		}

		pterm.DefaultBulletList.WithItems(statusItems).Render()

		return nil
	},
}

var daemonJoinCommand = &cli.Command{
	Name:      "join",
	Usage:     "Connect to Husarnet group with given join code and with specified hostname",
	ArgsUsage: "[join code] [device name]",
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
		callDaemonPost[EmptyResult]("/control/join", params)
		printSuccess("Successfully registered a join request")
		waitBaseANY()
		waitWebsetup()
		waitJoined()
		return nil
	},
}

var daemonSetupServerCommand = &cli.Command{
	Name:      "setup-server",
	Usage:     "Connect your Husarnet device to different Husarnet infrastructure",
	ArgsUsage: "[dashboard fqdn]",
	Action: func(ctx *cli.Context) error {
		if ctx.Args().Len() < 1 {
			fmt.Println("you need to provide address of the dashboard")
			return nil
		}

		domain := ctx.Args().Get(0)

		params := url.Values{
			"domain": {domain},
		}
		callDaemonPost[EmptyResult]("/control/change-server", params)
		printSuccess(fmt.Sprintf("Successfully requested a change to %s server", domain))
		printInfo("This action requires you to restart the daemon in order to use the new value")
		runSubcommand(true, "sudo", "systemctl", "restart", "husarnet")
		waitDaemon()
		return nil
	},
}

var daemonWhitelistCommand = &cli.Command{
	Name:  "whitelist",
	Usage: "Manage whitelist on the device.",
	Subcommands: []*cli.Command{
		{
			Name:      "enable",
			Aliases:   []string{"on"},
			Usage:     "enable whitelist",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				callDaemonPost[EmptyResult]("/control/whitelist/enable", url.Values{})
				printSuccess("Enabled the whitelist")
				return nil
			},
		},
		{
			Name:      "disable",
			Aliases:   []string{"off"},
			Usage:     "disable whitelist",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				callDaemonPost[EmptyResult]("/control/whitelist/whitelist", url.Values{})
				printSuccess("Disabled the whitelist")
				return nil
			},
		},
		{
			Name:      "ls",
			Aliases:   []string{"show", "dir"},
			Usage:     "list entries on the whitelist",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				status := getDaemonStatus()
				pterm.DefaultBulletList.WithItems(getWhitelistBullets(status)).Render()

				return nil
			},
		},
		{
			Name:      "add",
			Usage:     "Add a device to your whitelist by Husarnet address",
			ArgsUsage: "[device's ip address]",
			Action: func(ctx *cli.Context) error {
				if ctx.Args().Len() < 1 {
					fmt.Println("you need to provide Husarnet address of the device")
					return nil
				}
				addr := ctx.Args().Get(0)

				params := url.Values{
					"address": {addr},
				}
				callDaemonPost[EmptyResult]("/control/whitelist/add", params)
				printSuccess(fmt.Sprintf("Added %s to whitelist", addr))
				return nil
			},
		},
		{
			Name:      "rm",
			Usage:     "Remove device from the whitelist",
			ArgsUsage: "[device's ip address]",
			Action: func(ctx *cli.Context) error {
				if ctx.Args().Len() < 1 {
					fmt.Println("you need to provide Husarnet address of the device")
					return nil
				}
				addr := ctx.Args().Get(0)

				params := url.Values{
					"address": {addr},
				}
				callDaemonPost[EmptyResult]("/control/whitelist/rm", params)
				printSuccess(fmt.Sprintf("Removed %s from whitelist", addr))
				return nil
			},
		},
	},
}

var daemonWaitCommand = &cli.Command{
	Name:  "wait",
	Usage: "Wait until certain events occur. If no events provided will wait for as many elements as it can (the best case scenario). Husarnet will continue working even if some of those elements are unreachable, so consider narrowing your search down a bit.",
	Action: func(ctx *cli.Context) error {
		ignoreExtraArguments(ctx)
		waitBaseANY()
		waitBaseUDP()
		waitWebsetup()
		return nil
	},
	Subcommands: []*cli.Command{
		{
			Name:      "base",
			Usage:     "Wait until there is a base-server connection established (via any protocol)",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				ignoreExtraArguments(ctx)
				waitBaseANY()
				return nil
			},
			Subcommands: []*cli.Command{
				{
					Name:      "udp",
					Usage:     "Wait until there is a base-server connection established via UDP. This is the best case scenario. Husarnet will work even without it.",
					ArgsUsage: " ", // No arguments needed
					Action: func(ctx *cli.Context) error {
						ignoreExtraArguments(ctx)
						waitBaseUDP()
						return nil
					},
				},
			},
		},
		{
			Name:      "joinable",
			Usage:     "Wait until there is enough connectivity to join a network/adopt a device",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				ignoreExtraArguments(ctx)
				waitBaseANY()
				waitWebsetup()
				return nil
			},
		},
		{
			Name:      "joined",
			Usage:     "Wait until there is enough connectivity to join a network/adopt a device",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				ignoreExtraArguments(ctx)
				waitJoined()
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
		daemonStartCommand,
		daemonRestartCommand,
		daemonStopCommand,
		daemonWhitelistCommand,
		daemonWaitCommand,
	},
}
