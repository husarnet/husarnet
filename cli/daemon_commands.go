// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"net/url"
	"os"
	"runtime"
	"strings"
	"sync"

	"github.com/pterm/pterm"
	"github.com/urfave/cli/v3"
)

var daemonStartCommand = &cli.Command{
	Name:      "start",
	Aliases:   []string{"up"},
	Category:  CategoryDaemon,
	Usage:     "Start husarnet daemon",
	ArgsUsage: " ", // No arguments needed
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:        "wait",
			Aliases:     []string{"w"},
			Usage:       "wait until the daemon is running",
			Destination: &wait,
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
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
	Usage:     "Restart husarnet daemon",
	Category:  CategoryDaemon,
	ArgsUsage: " ", // No arguments needed
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:        "wait",
			Aliases:     []string{"w"},
			Usage:       "wait until the daemon is running",
			Destination: &wait,
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
		// Temporary solution for Windows, until we get rid of nssm
		if onWindows() {
			runSubcommand(false, "nssm", "restart", "husarnet")
			return nil
		}

		err := restartService()
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
	Usage:     "Stop husarnet daemon",
	Category:  CategoryDaemon,
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx context.Context, cmd *cli.Command) error {
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
	Action: func(ctx context.Context, cmd *cli.Command) error {
		if cmd.Bool("follow") {
			printStatusFollow(cmd)
		} else {
			status := getDaemonStatus()
			printStatus(cmd, status)
		}
		return nil

	},
}

var daemonSetupServerCommand = &cli.Command{
	Name:      "setup-server",
	Usage:     "Connect your Husarnet device to different instance of Husarnet infrastructure (eg. self-hosted instances)",
	ArgsUsage: "[dashboard fqdn]",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)

		domain := cmd.Args().Get(0)

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

		err := restartService()
		if err != nil {
			printWarning("Wasn't able to restart Husarnet Daemon. Try restarting the service manually.")
			return err
		}

		waitDaemon()

		return nil
	},
}

var daemonWhitelistCommand = &cli.Command{
	Name:     "whitelist",
	Usage:    "Manage whitelist on the device.",
	Category: CategoryDaemon,
	Commands: []*cli.Command{
		{
			Name:      "enable",
			Aliases:   []string{"on"},
			Usage:     "enable whitelist",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx context.Context, cmd *cli.Command) error {
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
			Action: func(ctx context.Context, cmd *cli.Command) error {
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
			Action: func(ctx context.Context, cmd *cli.Command) error {
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
			Action: func(ctx context.Context, cmd *cli.Command) error {
				requiredArgumentsNumber(cmd, 1)

				addr := makeCanonicalAddr(cmd.Args().Get(0))

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
			Action: func(ctx context.Context, cmd *cli.Command) error {
				requiredArgumentsNumber(cmd, 1)

				addr := makeCanonicalAddr(cmd.Args().Get(0))

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
	Name:     "hooks",
	Usage:    "Manage hooks on the device.",
	Category: CategoryUtils,
	Commands: []*cli.Command{
		{
			Name:      "enable",
			Aliases:   []string{"on"},
			Usage:     "enable hooks",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx context.Context, cmd *cli.Command) error {
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
			Action: func(ctx context.Context, cmd *cli.Command) error {
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
			Action: func(ctx context.Context, cmd *cli.Command) error {
				status := getDaemonStatus()
				handleStandardResult(status.StdResult)
				printHooksStatus(status)

				return nil
			},
		},
	},
}

var daemonWaitCommand = &cli.Command{
	Name:     "wait",
	Usage:    "Wait until certain events occur. If no events provided will wait for as many elements as it can.",
	Category: CategoryUtils,
	Action: func(ctx context.Context, cmd *cli.Command) error {
		ignoreExtraArguments(cmd)
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
	Commands: []*cli.Command{
		{
			Name:      "daemon",
			Aliases:   []string{"ready"},
			Usage:     "Wait until the deamon is able to return it's own status",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx context.Context, cmd *cli.Command) error {
				ignoreExtraArguments(cmd)
				return waitDaemon()
			},
		},
		{
			Name:      "base",
			Usage:     "Wait until there is a base-server connection established (via any protocol)",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx context.Context, cmd *cli.Command) error {
				ignoreExtraArguments(cmd)
				return waitBaseANY()
			},
			Commands: []*cli.Command{
				{
					Name:      "udp",
					Usage:     "Wait until there is a base-server connection established via UDP. This is the best case scenario. Husarnet will work even without it.",
					ArgsUsage: " ", // No arguments needed
					Action: func(ctx context.Context, cmd *cli.Command) error {
						ignoreExtraArguments(cmd)
						return waitBaseUDP()
					},
				},
			},
		},
		{
			Name:      "joinable",
			Usage:     "Wait until there is enough connectivity to join to a network",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx context.Context, cmd *cli.Command) error {
				ignoreExtraArguments(cmd)

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
			Action: func(ctx context.Context, cmd *cli.Command) error {
				ignoreExtraArguments(cmd)
				return waitJoined()
			},
		},
		{
			Name:      "host",
			Usage:     "Wait until there is an established connection to a given host",
			ArgsUsage: "[device name or ip]",
			Action: func(ctx context.Context, cmd *cli.Command) error {
				requiredArgumentsNumber(cmd, 1)
				return waitHost(cmd.Args().First())
			},
		},
		{
			Name:      "hostnames",
			Usage:     "Wait until given hosts are known to the system",
			ArgsUsage: "[device name] […]",
			Action: func(ctx context.Context, cmd *cli.Command) error {
				minimumArguments(cmd, 1)
				return waitHostnames(cmd.Args().Slice())
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
	Usage:     "Generate a valid identity file",
	Category:  CategoryUtils,
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
	Action: func(ctx context.Context, cmd *cli.Command) error {
		ignoreExtraArguments(cmd)

		var newId string

		if cmd.String("include") != "" {
			needle := cmd.String("include")
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

		if cmd.Bool("save") {
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

			if !cmd.Bool("silent") {
				restartDaemonWithConfirmationPrompt()
			}
		} else {
			pterm.Printf(newId)

			if !cmd.Bool("silent") {
				printInfo("In order to use it save the line above as " + getDaemonIdPath())
			}
		}

		return nil
	},
}

var daemonCommand = &cli.Command{
	Name:  "daemon",
	Usage: "control the local daemon",
	Commands: []*cli.Command{
		daemonStatusCommand,
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
