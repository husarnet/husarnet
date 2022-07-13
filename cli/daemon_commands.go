// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"net/url"

	"github.com/urfave/cli/v2"
)

var daemonStatusCommand = &cli.Command{
	Name:  "status",
	Usage: "Display current connectivity status",
	Action: func(ctx *cli.Context) error {
		fmt.Printf("status: %v\n", getDaemonStatus()) // TODO
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

var daemonCommand = &cli.Command{
	Name:  "daemon",
	Usage: "Control the local daemon",
	Subcommands: []*cli.Command{
		daemonStatusCommand,
		daemonJoinCommand,
		daemonSetupServerCommand,
		daemonWhitelistCommand,
	},
}
