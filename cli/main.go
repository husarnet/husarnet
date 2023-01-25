// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

// Tag for code generator. Do not delete.
//go:generate go run github.com/Khan/genqlient

import (
	"log"
	"os"

	"github.com/urfave/cli/v2"
)

var defaultDashboard = "app.husarnet.com"
var defaultDaemonAPIPort = 16216

var husarnetDashboardFQDN string
var husarnetDaemonAPIPort = 0
var verboseLogs bool
var wait bool
var nonInteractive bool

func main() {
	app := &cli.App{
		Name:                 "Husarnet CLI",
		HelpName:             "husarnet",
		Usage:                "Manage your Husarnet groups and devices from your terminal",
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
		},
		Before: func(ctx *cli.Context) error {
			initTheme()

			return nil
		},
		Commands: []*cli.Command{
			dashboardCommand,
			daemonCommand,

			daemonStartCommand,
			daemonRestartCommand,
			daemonStopCommand,

			daemonStatusCommand,
			joinCommand,
			daemonSetupServerCommand,

			dashboardLoginCommand,

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
