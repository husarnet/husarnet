// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"fmt"
	"log"
	"net"
	"os"

	"github.com/urfave/cli/v3"
)

var defaultDashboard = "app.husarnet.com"
var defaultDaemonAPIIp = "127.0.0.1"
var defaultDaemonAPIPort int64 = 16216

var husarnetDashboardFQDN string
var husarnetDaemonAPIIp = ""
var husarnetDaemonAPIPort int64 = 0
var verboseLogs bool
var wait bool
var nonInteractive bool
var rawJson bool
var secret string

func main() {
	cmd := &cli.Command{
		Name: "husarnet",
		//HelpName: "husarnet",
		Description: `
This is Husarnet CLI (command-line interface), which is invoked with 'husarnet' command.
It's primary purpose is to query and manage daemon process ('husarnet-daemon') running 
on the current machine. Additionally, given sufficient permissions, it can be also be used 
to manage your other Husarnet devices and even your entire Husarnet network (possibly 
eliminating the need to ever use the web interface under https://dashboard.husarnet.com). 
For the details on what can be done with the CLI, visit: https://husarnet.com/docs/cli-guide.`,
		Usage:                 "manage your Husarnet network",
		EnableShellCompletion: true,
		Flags: []cli.Flag{
			&cli.StringFlag{
				Name:        "dashboard_fqdn",
				Aliases:     []string{"d"},
				Value:       getDaemonsDashboardFqdn(),
				Usage:       "FQDN for your dashboard instance.",
				Sources:     cli.EnvVars("HUSARNET_DASHBOARD_FQDN"),
				Destination: &husarnetDashboardFQDN,
			},
			&cli.IntFlag{
				Name:        "daemon_api_port",
				Aliases:     []string{"p"},
				Value:       defaultDaemonAPIPort,
				Usage:       "port your Husarnet Daemon is listening at",
				Sources:     cli.EnvVars("HUSARNET_DAEMON_API_PORT"),
				Destination: &husarnetDaemonAPIPort,
				Action: func(ctx context.Context, cmd *cli.Command, v int64) error {
					if v < 0 || v > 65535 {
						return fmt.Errorf("invalid port %d", v)
					}
					return nil
				},
			},
			&cli.StringFlag{
				Name:        "daemon_api_address",
				Aliases:     []string{"a"},
				Value:       defaultDaemonAPIIp,
				Usage:       "IP address your Husarnet Daemon is listening at",
				Sources:     cli.EnvVars("HUSARNET_DAEMON_API_ADDRESS"),
				Destination: &husarnetDaemonAPIIp,
				Action: func(ctx context.Context, cmd *cli.Command, v string) error {
					if net.ParseIP(v) == nil {
						return fmt.Errorf("invalid IP address %s", v)
					}
					return nil
				},
			},
			&cli.StringFlag{
				Name:        "secret",
				Aliases:     []string{"s"},
				Value:       "",
				Usage:       "swap secret for a different one",
				Sources:     cli.EnvVars("SECRET"),
				Destination: &secret,
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
			&cli.BoolFlag{
				Name:        "json",
				Usage:       "return raw json response from the API. This is useful for scripts or piping to other tools",
				Destination: &rawJson,
				Value:       false,
			},
		},
		Before: func(ctx context.Context, cmd *cli.Command) (context.Context, error) {
			initTheme()
			return ctx, nil
		},
		Commands: []*cli.Command{
			daemonStatusCommand,
			daemonStartCommand,
			daemonRestartCommand,
			daemonStopCommand,
			daemonIpCommand,
			daemonCommand,

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
