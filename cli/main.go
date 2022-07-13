// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

// Tag for code generator. Do not delete.
//go:generate go run github.com/Khan/genqlient

import (
	"fmt"
	"log"
	"os"

	"github.com/urfave/cli/v2"
)

var husarnetDashboardFQDN string
var husarnetDaemonAPIPort int
var verboseLogs bool

func main() {
	app := &cli.App{
		Name:     "Husarnet CLI",
		HelpName: "husarnet",
		Usage:    "Manage your Husarnet groups and devices from your terminal",
		Flags: []cli.Flag{
			&cli.StringFlag{
				Name:        "dashboard_fqdn",
				Aliases:     []string{"d"},
				Value:       "app.husarnet.com",
				Usage:       "FQDN for your dashboard instance.",
				EnvVars:     []string{"HUSARNET_DASHBOARD_FQDN"},
				Destination: &husarnetDashboardFQDN,
			},
			&cli.IntFlag{
				Name:        "daemon_api_port",
				Aliases:     []string{"p"},
				Value:       16216,
				Usage:       "Port your Husarnet Daemon is listening at",
				EnvVars:     []string{"HUSARNET_DAEMON_API_PORT"},
				Destination: &husarnetDaemonAPIPort,
			},
			&cli.BoolFlag{
				Name:        "verbose",
				Aliases:     []string{"v"},
				Usage:       "Show verbose logs (for debugging purposes)",
				Destination: &verboseLogs,
			},
		},
		Commands: []*cli.Command{
			dashboardCommand,
			daemonCommand,
			daemonStatusCommand,
			daemonJoinCommand,
			daemonSetupServerCommand,
			dashboardLoginCommand,
			{
				Name:  "version",
				Usage: "print the version of the CLI and also of the daemon, if available",
				Action: func(ctx *cli.Context) error {
					fmt.Printf("husarnet (CLI): %s\n", version)
					fmt.Printf("husarnet-daemon (binary): %s\n", getDaemonBinaryVersion())
					fmt.Printf("husarnet-daemon (running): %s\n", getDaemonRunningVersion())

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
