// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"net/url"
	"os"
	"runtime"
	"strconv"
	"strings"
	"sync"

	"github.com/pterm/pterm"
	"github.com/urfave/cli/v2"
)

var daemonStartCommand = &cli.Command{
	Name:      "start",
	Aliases:   []string{"up"},
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
		if isDaemonRunning() {
			printInfo("Husarnet Daemon is already running. If you want to restart it, use `husarnet daemon restart` command.")
			return nil
		}

		// Temporary solution for Windows, until we get rid of nssm
		if onWindows() {
			runSubcommand(false, "nssm", "start", "husarnet")
			return nil
		}

		ensureServiceInstalled()
		err := ServiceObject.Start()
		if err != nil {
			printError("Error starting husarnet-daemon: %s", err)
		} else {
			printSuccess("Started husarnet-daemon")
		}

		if wait {
			waitDaemon()
		}

		return nil
	},
}

var daemonRestartCommand = &cli.Command{
	Name:      "restart",
	Aliases:   []string{"down"},
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
		// Temporary solution for Windows, until we get rid of nssm
		if onWindows() {
			runSubcommand(false, "nssm", "restart", "husarnet")
			return nil
		}

		ensureServiceInstalled()
		err := ServiceObject.Restart()
		if err != nil {
			printError("Error restarting husarnet-daemon: %s", err)
		} else {
			printSuccess("Restarted husarnet-daemon")
		}

		if wait {
			waitDaemon()
		}

		return nil
	},
}

var daemonStopCommand = &cli.Command{
	Name:      "stop",
	Aliases:   []string{"down"},
	Usage:     "stop husarnet daemon",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx *cli.Context) error {
		if onWindows() {
			runSubcommand(false, "nssm", "stop", "husarnet")
			return nil
		}

		ensureServiceInstalled()
		err := ServiceObject.Stop()
		if err != nil {
			printError("Error stopping husarnet-daemon: %s", err)
		} else {
			printSuccess("Stopped husarnet-daemon")
		}

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
		&cli.BoolFlag{
			Name:    "follow",
			Aliases: []string{"f"},
			Usage:   "show more information",
		},
	},
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx *cli.Context) error {
		if ctx.Bool("follow") {
			printStatusFollow(ctx)
		} else {
			status := getDaemonStatus()
			printStatus(ctx, status)
		}
		return nil

	},
}

var daemonLogsCommand = &cli.Command{
	Name:  "logs",
	Usage: "Display and manage logs settings",
	Subcommands: []*cli.Command{
		{
			Name:      "settings",
			Aliases:   []string{"status"},
			Usage:     "print logs settings",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				settings := callDaemonGet[LogsSettings]("/api/logs/settings").Result
				handleStandardResult(settings.StdResult)
				printSuccess("Logs verbosity level: " + strconv.Itoa(settings.VerbosityLevel))
				printSuccess("Logs maximum size: " + strconv.Itoa(settings.Size))
				printSuccess("Logs current size: " + strconv.Itoa(settings.CurrentSize))

				return nil
			},
		},
		{
			Name:      "print",
			Aliases:   []string{"get"},
			Usage:     "print logs",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				logsResponse := callDaemonGet[LogsResponse]("/api/logs/get").Result
				handleStandardResult(logsResponse.StdResult)

				for _, line := range logsResponse.Logs {
					pterm.Println(line)
				}

				return nil
			},
		},
		{
			Name:      "verbosity",
			Aliases:   []string{""},
			Usage:     "change logs verbosity level",
			ArgsUsage: "[0-4]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsNumber(ctx, 1)

				verbosityStr := ctx.Args().Get(0)
				verbosity, error := strconv.Atoi(verbosityStr)
				if error == nil && verbosity <= 4 && verbosity >= 0 {
					stdResult := callDaemonPost[StandardResult]("/api/logs/settings", url.Values{
						"verbosity": {verbosityStr},
					}).Result
					handleStandardResult(stdResult)
					printSuccess("Verbosity level changed")
					return nil
				}
				printError("Verbosity provided should belong to range 0-4")
				return nil
			},
		},
		{
			Name:      "size",
			Aliases:   []string{""},
			Usage:     "change size of in memory stored logs",
			ArgsUsage: "[10-1000]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsNumber(ctx, 1)

				sizeStr := ctx.Args().Get(0)
				size, error := strconv.Atoi(sizeStr)
				if error == nil && size <= 1000 && size >= 10 {
					stdResult := callDaemonPost[StandardResult]("/api/logs/settings", url.Values{
						"size": {sizeStr},
					}).Result
					handleStandardResult(stdResult)
					printSuccess("In memory logs size changed")
					return nil
				}
				printError("Size provided should belong to range 10-1000")
				return nil
			},
		},
	},
}

var daemonSetupServerCommand = &cli.Command{
	Name:      "setup-server",
	Aliases:   []string{"change-dashboard"},
	Usage:     "Connect your Husarnet device to different Husarnet infrastructure",
	ArgsUsage: "[dashboard fqdn]",
	Action: func(ctx *cli.Context) error {
		requiredArgumentsNumber(ctx, 1)

		domain := ctx.Args().Get(0)

		callDaemonPost[EmptyResult]("/api/change-server", url.Values{
			"domain": {domain},
		})

		printSuccess("Successfully requested a change to %s server", pterm.Bold.Sprint(domain))
		printWarning("This action requires you to restart the daemon in order to use the new value")

		if !askForConfirmation("Do you want to restart Husarnet daemon now?") {
			dieEmpty()
		}

		if onWindows() {
			runSubcommand(false, "nssm", "restart", "husarnet")
			return nil
		}

		ensureServiceInstalled()
		err := ServiceObject.Restart()
		if err != nil {
			printWarning("Wasn't able to restart Husarnet Daemon. Try restarting the service manually.")
			return err
		}

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
				stdResult := callDaemonPost[StandardResult]("/api/whitelist/enable", url.Values{}).Result
				handleStandardResult(stdResult)
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
				stdResult := callDaemonPost[StandardResult]("/api/whitelist/disable", url.Values{}).Result
				handleStandardResult(stdResult)
				printSuccess("Disabled the whitelist")

				return nil
			},
		},
		{
			Name:      "ls",
			Aliases:   []string{"show", "dir", "get"},
			Usage:     "list entries on the whitelist",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				status := getDaemonStatus()
				handleStandardResult(status.StdResult)
				printWhitelist(status, false)

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

				stdResult := callDaemonPost[StandardResult]("/api/whitelist/add", url.Values{
					"address": {addr},
				}).Result
				handleStandardResult(stdResult)
				printSuccess("Added %s to whitelist", addr)

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

				stdResult := callDaemonPost[StandardResult]("/api/whitelist/rm", url.Values{
					"address": {addr},
				}).Result
				handleStandardResult(stdResult)
				printSuccess("Removed %s from whitelist", addr)

				return nil
			},
		},
	},
}

var daemonHooksCommand = &cli.Command{
	Name:  "hooks",
	Usage: "Manage hooks on the device.",
	Subcommands: []*cli.Command{
		{
			Name:      "enable",
			Aliases:   []string{"on"},
			Usage:     "enable hooks",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				stdResult := callDaemonPost[StandardResult]("/api/hooks/enable", url.Values{}).Result
				handleStandardResult(stdResult)
				printSuccess("Enabled hooks")

				return nil
			},
		},
		{
			Name:      "disable",
			Aliases:   []string{"off"},
			Usage:     "disable hooks",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				stdResult := callDaemonPost[StandardResult]("/api/hooks/disable", url.Values{}).Result
				handleStandardResult(stdResult)
				printSuccess("Disabled hooks")

				return nil
			},
		},
		{
			Name:      "show",
			Aliases:   []string{"check", "ls"},
			Usage:     "check if hooks are enabled",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				status := getDaemonStatus()
				handleStandardResult(status.StdResult)
				printHooksStatus(status)

				return nil
			},
		},
	},
}

var daemonNotificationCommand = &cli.Command{
	Name:  "notifications",
	Usage: "Manage notifications on the device.",
	Subcommands: []*cli.Command{
		{
			Name:      "enable",
			Aliases:   []string{"on"},
			Usage:     "enable notifications",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				stdResult := callDaemonPost[StandardResult]("/api/notifications/enable", url.Values{}).Result
				handleStandardResult(stdResult)
				printSuccess("Enabled notifications")

				return nil
			},
		},
		{
			Name:      "disable",
			Aliases:   []string{"off"},
			Usage:     "disable notifications",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				stdResult := callDaemonPost[StandardResult]("/api/notifications/disable", url.Values{}).Result
				handleStandardResult(stdResult)
				printSuccess("Disabled notifications")

				return nil
			},
		},
		{
			Name:      "show",
			Aliases:   []string{"check", "ls"},
			Usage:     "check if notifications are enabled",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				status := getDaemonStatus()
				handleStandardResult(status.StdResult)
				printNotificationsStatus(status.StdResult)

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
			Usage:     "Wait until there is enough connectivity to join to a network",
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
			Usage:     "Wait until there the daemon has joined the network",
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

func genId(needle string, stripHaystack bool) (string, bool) {
	result := getDaemonBinaryGenIdOutput()

	var haystack = strings.SplitN(result, " ", 2)[0]
	if stripHaystack {
		haystack = strings.ReplaceAll(haystack, ":", "")
	}

	printInfo("Trying %s...", haystack)

	if strings.Contains(haystack, needle) {
		return result, true
	}

	return "", false
}

func genParallel(needle string, stripHaystack bool) string {
	var wg sync.WaitGroup
	semaphore := make(chan struct{}, runtime.NumCPU())

	var done bool
	var result string
	var lock sync.Mutex

	for {
		semaphore <- struct{}{}

		wg.Add(1)
		go func() {
			defer wg.Done()

			resultTmp, success := genId(needle, stripHaystack)

			if success {
				lock.Lock()
				done = true
				result = resultTmp
				lock.Unlock()
			}

			<-semaphore
		}()

		lock.Lock()
		if done {
			break
		}
		lock.Unlock()
	}

	wg.Wait()
	return result
}

var daemonGenIdCommand = &cli.Command{
	Name:      "genid",
	Aliases:   []string{"genID", "genId", "gen-id"},
	Usage:     "Generate a valid identity file",
	ArgsUsage: " ", // No arguments needed
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:  "silent",
			Usage: "Suppress all help messages so it can be directly piped into " + getDaemonIdPath(),
		},
		&cli.StringFlag{
			Name:  "include",
			Usage: "Generate identities until you find one that contains a given hex sequence. There are two basic formatting options. Either your string has to be a [a-f0-9]+ sequence (and colons will be added automatically), or, if it contains at least one colon it needs to be a valid fragment of a fully expanded IPv6 addres (i.e. :cafe:1234, ca:fe, cafe:0001).",
		},
		&cli.BoolFlag{
			Name:  "save",
			Usage: "Save the result in " + getDaemonIdPath(),
		},
	},
	Action: func(ctx *cli.Context) error {
		ignoreExtraArguments(ctx)

		var newId string

		if ctx.String("include") != "" {
			needle := ctx.String("include")
			stripHaystack := false

			if strings.Contains(needle, ":") {
				if !isHexString(strings.ReplaceAll(needle, ":", "")) {
					die("Your pattern is not a valid part of an IPv6 address")
				}
			} else {
				if !isHexString(needle) {
					die("Your pattern contains some characters that are not hexadecimal")
				}
				stripHaystack = true
			}

			newId = genParallel(needle, stripHaystack)
		} else {
			newId = getDaemonBinaryGenIdOutput()
		}

		if ctx.Bool("save") {
			err := os.WriteFile(getDaemonIdPath(), []byte(newId), 0600)
			if err != nil {
				printError("Error: could not save new ID: %s", err)

				// TODO: outdated function according to go docs
				if os.IsPermission(err) {
					rerunWithSudoOrDie()
				} else {
					dieEmpty()
				}
			}

			printInfo("Saved! In order to apply it you need to restart Husarnet Daemon!")
			restartDaemonWithConfirmationPrompt()
		} else {
			pterm.Printf(newId)

			if !ctx.Bool("silent") {
				printInfo("In order to use it save the line above as " + getDaemonIdPath())
			}
		}

		return nil
	},
}

var daemonCommand = &cli.Command{
	Name:  "daemon",
	Usage: "Control the local daemon",
	Subcommands: []*cli.Command{
		daemonStatusCommand,
		joinCommand,
		daemonLogsCommand,
		daemonNotificationCommand,
		daemonWaitCommand,
		daemonStartCommand,
		daemonStopCommand,
		daemonRestartCommand,
		daemonSetupServerCommand,
		daemonWhitelistCommand,
		daemonHooksCommand,
		daemonGenIdCommand,
		daemonServiceInstallCommand,
		daemonServiceUninstallCommand,
	},
}
