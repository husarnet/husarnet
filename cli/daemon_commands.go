// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"net/netip"
	"net/url"
	"sort"
	"strings"
	"time"

	"github.com/pterm/pterm"
	u "github.com/rjNemo/underscore"
	"github.com/urfave/cli/v2"
	"golang.org/x/text/cases"
	"golang.org/x/text/language"
)

type dumbObserver[T comparable] struct {
	lastValue  T
	changeHook func(oldValue, NewValue T)
}

func NewDumbObserver[T comparable](changeHook func(oldValue, NewValue T)) *dumbObserver[T] {
	observer := dumbObserver[T]{
		changeHook: changeHook,
	}

	return &observer
}

func (o *dumbObserver[T]) Update(newValue T) {
	if newValue != o.lastValue {
		o.changeHook(o.lastValue, newValue)
		o.lastValue = newValue
	}
}

func waitAction(message string, lambda func(*DaemonStatus) (bool, string)) error {
	spinner, _ := pterm.DefaultSpinner.Start(message)
	observer := NewDumbObserver(func(old, new string) {
		spinner.UpdateText(new)
	})

	failedCounter := 0
	for {
		failedCounter++
		if failedCounter > 60 {
			spinner.Fail("timeout")
			return fmt.Errorf("timeout")
		}

		response, err := getDaemonStatusRaw(false)
		if err != nil {
			spinner.Fail(err)
			time.Sleep(time.Second)
			continue
		}

		success, temporaryMessage := lambda(&response.Result)
		if success {
			observer.Update(message)
			spinner.Success()
			return nil
		}

		observer.Update(message + " " + temporaryMessage)

		time.Sleep(time.Second)
	}
}

func waitDaemon() error {
	return waitAction("Waiting until we can communicate with husarnet daemon…", func(status *DaemonStatus) (bool, string) { return true, "" })
}

func waitBaseANY() error {
	return waitAction("Waiting for Base server connection (any protocol)…", func(status *DaemonStatus) (bool, string) {
		return status.BaseConnection.Type == "TCP" || status.BaseConnection.Type == "UDP", ""
	})
}

func waitBaseUDP() error {
	return waitAction("Waiting for Base server connection (UDP)…", func(status *DaemonStatus) (bool, string) { return status.BaseConnection.Type == "UDP", "" })
}

func waitWebsetup() error {
	return waitAction("Waiting for websetup connection…", func(status *DaemonStatus) (bool, string) { return status.ConnectionStatus["websetup"], "" })
}

func waitJoined() error {
	return waitAction("Waiting until the device is joined…", func(status *DaemonStatus) (bool, string) { return status.IsJoined, "" })
}

func waitHost(hostnameOrIp string) error {
	hostname := ""
	addr, err := netip.ParseAddr(hostnameOrIp)
	if err != nil {
		hostname = hostnameOrIp
	}

	printInfo("Remember that in order to consider a connection established there need to be at least 1 attempt of connection using the regular networking stack - i.e. a single ping to a given host")

	return waitAction(pterm.Sprintf("Waiting until there's a connection to %s…", hostnameOrIp), func(status *DaemonStatus) (bool, string) {
		hostIp, present := status.HostTable[hostname]
		if hostname != "" && present {
			addr = hostIp
		}

		if !addr.IsValid() {
			return false, "peer addr is not known yet" // Multiple paths - invalid addr + host not present in host table is the most important one
		}

		peer := status.getPeerByAddr(addr)

		if peer == nil {
			return false, "unable to find peer in the peer list" // Unable to find peer
		}

		if !peer.IsActive {
			return false, "peer is not active yet" // Theres is no connection established yet (remember to try connecting to it though)
		}

		if !peer.IsSecure {
			return false, "secure connection has not yet been established"
		}

		return true, ""
	})
}

func waitHostnames(hostnames []string) error {
	return waitAction(pterm.Sprintf("Waiting until the following hostnames are known to: %s…", strings.Join(hostnames, ", ")), func(status *DaemonStatus) (bool, string) {
		for _, hostname := range hostnames {
			_, present := status.HostTable[hostname]
			if !present {
				return false, pterm.Sprintf("%s is unavailable", hostname)
			}
		}

		return true, ""
	})
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

func getWhitelistBullets(status DaemonStatus, verbose bool) []pterm.BulletListItem {
	statusItems := []pterm.BulletListItem{}

	sort.Slice(status.Whitelist, func(i, j int) bool { return status.Whitelist[i].String() < status.Whitelist[j].String() })

	for _, address := range status.Whitelist {
		statusItems = append(statusItems, makeBullet(1, address.StringExpanded(), flashyStyle))

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
			statusItems = append(statusItems, makeBullet(2, "Role: websetup", defaultStyle))
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

			if verbose && len(peer.TargetAddresses) > 0 {
				statusItems = append(statusItems, makeBullet(2, fmt.Sprintf("Advertised network addresses: %s", strings.Join(u.Map(peer.TargetAddresses, func(it netip.AddrPort) string { return it.String() }), " ")), defaultStyle))
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
	Name:  "status",
	Usage: "Display current connectivity status",
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:    "verbose",
			Aliases: []string{"v"},
			Usage:   "show more information",
		},
	},
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx *cli.Context) error {
		verbose := verboseLogs || ctx.Bool("verbose")

		status := getDaemonStatus()

		versionStyle := casualStyle
		if version != status.Version {
			versionStyle = errorStyle
		}

		dashboardStyle := casualStyle
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
		}

		if status.IsDirty {
			statusItems = append(statusItems, makeBullet(0, "Daemon dirty flag is set. You need to restart husarnet-daemon in order to reflect the current settings", errorStyle))
		} else {
			statusItems = append(statusItems, makeBullet(0, "Daemon is not dirty", defaultStyle))
		}

		statusItems = append(statusItems,
			makeBullet(0, "Husarnet address", defaultStyle),
			makeBullet(1, status.LocalIP.String(), greenStyle), // This is on a separate line and without a bullet so it can be easily copied with a triple cli)k
			makeBullet(0, fmt.Sprintf("Local hostname: %s", status.LocalHostname), defaultStyle),
			makeBullet(0, "Whitelist", defaultStyle),
		)

		statusItems = append(statusItems, getWhitelistBullets(status, verbose)...)

		statusItems = append(statusItems, makeBullet(0, "Connection status", defaultStyle))

		// As UDP is the desired one - warn wbout everything that's not it
		var baseConnectionStyle *pterm.Style

		if status.BaseConnection.Type == "UDP" {
			baseConnectionStyle = casualStyle
		} else if status.BaseConnection.Type == "TCP" {
			baseConnectionStyle = warningStyle
		} else {
			baseConnectionStyle = errorStyle
		}
		statusItems = append(statusItems, makeBullet(1, fmt.Sprintf("Base server: %s:%v (%s)", status.BaseConnection.Address, status.BaseConnection.Port, status.BaseConnection.Type), baseConnectionStyle))

		if verbose {
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

		if verbose {
			statusItems = append(statusItems, makeBullet(0, "User settings", defaultStyle))
			for settingName, settingValue := range status.UserSettings {
				statusItems = append(statusItems, makeBullet(1, fmt.Sprintf("%s: %v", settingName, settingValue), defaultStyle))
			}
		}

		pterm.DefaultBulletList.WithItems(statusItems).Render()

		return nil
	},
}

var daemonSetupServerCommand = &cli.Command{
	Name:      "setup-server",
	Usage:     "Connect your Husarnet device to different Husarnet infrastructure",
	ArgsUsage: "[dashboard fqdn]",
	Action: func(ctx *cli.Context) error {
		requiredArgumentsNumber(ctx, 1)

		domain := ctx.Args().Get(0)

		callDaemonPost[EmptyResult]("/api/change-server", url.Values{
			"domain": {domain},
		})

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
				callDaemonPost[EmptyResult]("/api/whitelist/enable", url.Values{})
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
				callDaemonPost[EmptyResult]("/api/whitelist/whitelist", url.Values{})
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
				pterm.DefaultBulletList.WithItems(getWhitelistBullets(status, false)).Render()

				return nil
			},
		},
		{
			Name:      "add",
			Usage:     "Add a device to your whitelist by Husarnet address",
			ArgsUsage: "[device's ip address]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsNumber(ctx, 1)

				addr := makeCannonicalAddr(ctx.Args().Get(0))

				callDaemonPost[EmptyResult]("/api/whitelist/add", url.Values{
					"address": {addr},
				})
				printSuccess(fmt.Sprintf("Added %s to whitelist", addr))

				return nil
			},
		},
		{
			Name:      "rm",
			Usage:     "Remove device from the whitelist",
			ArgsUsage: "[device's ip address]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsNumber(ctx, 1)

				addr := makeCannonicalAddr(ctx.Args().Get(0))

				callDaemonPost[EmptyResult]("/api/whitelist/rm", url.Values{
					"address": {addr},
				})
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
		err := waitBaseANY()
		if err != nil {
			return err
		}

		err = waitBaseUDP()
		if err != nil {
			return err
		}

		return waitWebsetup()
	},
	Subcommands: []*cli.Command{
		{
			Name:      "daemon",
			Aliases:   []string{"ready"},
			Usage:     "Wait until the deamon is able to return it's own status",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				ignoreExtraArguments(ctx)
				return waitDaemon()
			},
		},
		{
			Name:      "base",
			Usage:     "Wait until there is a base-server connection established (via any protocol)",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				ignoreExtraArguments(ctx)
				return waitBaseANY()
			},
			Subcommands: []*cli.Command{
				{
					Name:      "udp",
					Usage:     "Wait until there is a base-server connection established via UDP. This is the best case scenario. Husarnet will work even without it.",
					ArgsUsage: " ", // No arguments needed
					Action: func(ctx *cli.Context) error {
						ignoreExtraArguments(ctx)
						return waitBaseUDP()
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

				err := waitBaseANY()
				if err != nil {
					return err
				}

				return waitWebsetup()
			},
		},
		{
			Name:      "joined",
			Usage:     "Wait until there is enough connectivity to join a network/adopt a device",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				ignoreExtraArguments(ctx)
				return waitJoined()
			},
		},
		{
			Name:      "host",
			Usage:     "Wait until there is an established connection to a given host",
			ArgsUsage: "[device name or ip]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsNumber(ctx, 1)
				return waitHost(ctx.Args().First())
			},
		},
		{
			Name:      "hostnames",
			Usage:     "Wait until given hosts are known to the system",
			ArgsUsage: "[device name] […]",
			Action: func(ctx *cli.Context) error {
				minimumArguments(ctx, 1)
				return waitHostnames(ctx.Args().Slice())
			},
		},
	},
}

var daemonCommand = &cli.Command{
	Name:  "daemon",
	Usage: "Control the local daemon",
	Subcommands: []*cli.Command{
		daemonStatusCommand,
		joinCommand,
		daemonSetupServerCommand,
		daemonStartCommand,
		daemonRestartCommand,
		daemonStopCommand,
		daemonWhitelistCommand,
		daemonWaitCommand,
	},
}
