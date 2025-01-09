// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"fmt"
	"github.com/husarnet/husarnet/cli/v2/config"
	"github.com/husarnet/husarnet/cli/v2/utils"
	"log"
	"net"
	"os"

	"github.com/urfave/cli/v3"
)

var verboseLogs bool
var wait bool
var nonInteractive bool
var rawJson bool

const (
	CategoryDaemon = "DAEMON MANAGEMENT"
	CategoryUtils  = "UTILITIES"
	CategoryApi    = "DASHBOARD API ACCESS"
)

func main() {
	cli.RootCommandHelpTemplate = rootTemplate

	cmd := &cli.Command{
		Name: "husarnet",
		Description: `This is Husarnet CLI (command-line interface), which is invoked with 'husarnet' command.
It's primary purpose is to query and manage daemon process ('husarnet-daemon') running 
on the current machine. Additionally, given sufficient permissions, it can be also be used 
to manage your other Husarnet devices and even your entire Husarnet network (possibly 
eliminating the need to ever use the web interface under https://dashboard.husarnet.com).

For the details on what can be done with the CLI, visit: https://husarnet.com/docs/cli-guide.`,
		Usage:                 "Manage your Husarnet network",
		EnableShellCompletion: true,
		Flags: []cli.Flag{
			&cli.IntFlag{
				Name:    config.DaemonApiPortFlagName,
				Aliases: []string{"p"},
				Usage:   "port your Husarnet Daemon is listening at",
				Sources: cli.EnvVars(utils.EnvVarName(config.DaemonApiPortFlagName)),
				Action: func(ctx context.Context, cmd *cli.Command, v int64) error {
					if v < 0 || v > 65535 {
						return fmt.Errorf("invalid port %d", v)
					}
					return nil
				},
			},
			&cli.StringFlag{
				Name:    config.DaemonApiAddressFlagName,
				Aliases: []string{"a"},
				Usage:   "IP address your Husarnet Daemon is listening at",
				Sources: cli.EnvVars(utils.EnvVarName(config.DaemonApiAddressFlagName)),
				Action: func(ctx context.Context, cmd *cli.Command, v string) error {
					if net.ParseIP(v) == nil {
						return fmt.Errorf("invalid IP address %s", v)
					}
					return nil
				},
			},
			&cli.StringFlag{
				Name:    config.DaemonApiSecretFlagName,
				Aliases: []string{"s"},
				Value:   "",
				Usage:   "swap daemon API secret for a different one",
				Sources: cli.EnvVars(utils.EnvVarName(config.DaemonApiSecretFlagName)),
			},
			&cli.BoolFlag{
				Name:        "verbose",
				Aliases:     []string{"v"},
				Usage:       "show verbose logs (for debugging purposes)",
				Destination: &verboseLogs,
			},
			&cli.BoolFlag{
				Name:        "noninteractive",
				Aliases:     []string{"n", "non-interactive"},
				Usage:       "assume running in a non-interactive session, always select default action for all of the prompts",
				Destination: &nonInteractive,
				Value:       false,
			},
			// TODO: not every command respects this flag
			&cli.BoolFlag{
				Name:        "json",
				Usage:       "return raw json response from the API. This is useful for scripts or piping to other tools",
				Destination: &rawJson,
				Value:       false,
			},
		},
		Before: func(ctx context.Context, cmd *cli.Command) (context.Context, error) {
			config.Init(cmd)
			initTheme()
			return ctx, nil
		},
		Commands: []*cli.Command{
			daemonCommand, // daemon "umbrella" command is needed here for integration tests, will _maybe_ be eventually deleted/rearranged
			daemonStatusCommand,
			daemonStartCommand,
			daemonRestartCommand,
			daemonStopCommand,
			daemonIpCommand,
			daemonWaitCommand,
			daemonWhitelistCommand,
			daemonHooksCommand,
			daemonGenIdCommand,
			daemonServiceInstallCommand,
			daemonServiceUninstallCommand,
			claimCommand,
			tokenCommand,
			groupCommands,
			deviceCommands,

			daemonSetupServerCommand,
			versionCommand,
		},
	}

	err := cmd.Run(context.Background(), os.Args)
	if err != nil {
		log.Fatal(err)
	}
}
