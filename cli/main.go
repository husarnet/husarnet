// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"log"
	"net"
	"os"

	"github.com/urfave/cli/v2"
)

var defaultDashboard = "app.husarnet.com"
var defaultDaemonAPIIp = "127.0.0.1"
var defaultDaemonAPIPort = 16216

var husarnetDashboardFQDN string
var husarnetDaemonAPIIp = ""
var husarnetDaemonAPIPort = 0
var verboseLogs bool
var wait bool
var nonInteractive bool
var rawJson bool
var secret string

func main() {
	app := &cli.App{
		Name:     "Husarnet CLI",
		HelpName: "husarnet",
		Description: `
This is Husarnet CLI (command-line interface), which is invoked with 'husarnet' command.
It's primary purpose is to query and manage daemon process ('husarnet-daemon') running 
on the current machine. Additionally, given sufficient permissions, it can be also be used 
to manage your other Husarnet devices and even your entire Husarnet network (possibly 
eliminating the need to ever use the web interface under https://dashboard.husarnet.com). 
For the details on what can be done with the CLI, visit https://husarnet.com/docs/cli-guide
You can also simply just explore and play with the commands described below.`,
		Usage:                "manage your Husarnet network",
		EnableBashCompletion: true,
		Flags: []cli.Flag{
			&cli.StringFlag{
				Name:        "dashboard_fqdn",
				Aliases:     []string{"d"},
				Value:       getDaemonsDashboardFqdn(),
				Usage:       "FQDN for your dashboard instance.",
				EnvVars:     []string{"HUSARNET_DASHBOARD_FQDN"},
				Destination: &husarnetDashboardFQDN,
			},
			&cli.IntFlag{
				Name:        "daemon_api_port",
				Aliases:     []string{"p"},
				Value:       defaultDaemonAPIPort,
				Usage:       "port your Husarnet Daemon is listening at",
				EnvVars:     []string{"HUSARNET_DAEMON_API_PORT"},
				Destination: &husarnetDaemonAPIPort,
				Action: func(ctx *cli.Context, v int) error {
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
				EnvVars:     []string{"HUSARNET_DAEMON_API_ADDRESS"},
				Destination: &husarnetDaemonAPIIp,
				Action: func(ctx *cli.Context, v string) error {
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
				EnvVars:     []string{"SECRET"},
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
		Before: func(ctx *cli.Context) error {
			initTheme()
			return nil
		},
		Commands: []*cli.Command{
			daemonCommand,
			daemonStartCommand,
			daemonRestartCommand,
			daemonStopCommand,

			daemonStatusCommand,
			daemonIpCommand,

			dashboardTokenCommand,
			dashboardGroupCommand,
			dashboardDeviceCommand,

			claimCommand,
			daemonSetupServerCommand,

			{
				Name:  "version",
				Usage: "print the version of the CLI and also of the daemon, if available",
				Action: func(ctx *cli.Context) error {
					printVersion(getDaemonRunningVersion())

					return nil
				},
			},
		},
	}

	err := app.Run(os.Args)
	if err != nil {
		log.Fatal(err)
	}
}
