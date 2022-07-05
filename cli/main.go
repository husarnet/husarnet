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

// Global TODO:
// allow for removal/update via name, not an ID
// GET transport method for queries (defaults to POST)
// some user friendliness, validating input e.g. we expected number here

var graphqlServerURL string
var husarnetDaemonURL string = "http://127.0.0.1:16216"
var verboseLogs bool

func main() {
	app := &cli.App{
		Name:     "Husarnet CLI",
		HelpName: "husarnet",
		Usage:    "Manage your Husarnet groups and devices from your terminal",
		Flags: []cli.Flag{
			&cli.StringFlag{
				Name:        "url",
				Aliases:     []string{"u"},
				Value:       "https://app.husarnet.com/graphql",
				Usage:       "URL for your GraphQL endpoint. Remember to include scheme in this!",
				EnvVars:     []string{"HUSARNET_GRAPHQL_URL"},
				Destination: &graphqlServerURL,
			},
			&cli.BoolFlag{
				Name:        "verbose",
				Aliases:     []string{"v"},
				Usage:       "Show verbose logs (for debugging purposes)",
				Destination: &verboseLogs,
			},
		},
		Commands: []*cli.Command{
			{
				Name:   "version",
				Usage:  "print the version of the CLI and also of the daemon, if available",
				Action: handleVersion,
			},
			{
				Name:   "status",
				Usage:  "Display current connectivity status",
				Action: handleStatus,
			},
			{
				Name:   "join",
				Usage:  "Connect to Husarnet group with given join code and with specified hostname",
				Action: handleJoin,
			},
			{
				Name:   "setup-server",
				Usage:  "Connect your Husarnet device to different Husarnet infrastructure",
				Action: handleSetupServer,
			},
			{
				Name:  "whitelist",
				Usage: "Manage whitelist on the device.",
				Subcommands: []*cli.Command{
					{
						Name:   "ls",
						Usage:  "list entries on the whitelist",
						Action: handleWhitelistLs,
					},
					{
						Name:   "add",
						Usage:  "add a device to your whitelist by Husarnet address",
						Action: handleWhitelistAdd,
					},
					{
						Name:   "rm",
						Usage:  "remove device from the whitelist",
						Action: handleWhitelistRm,
					},
				},
			},
			{
				Name:  "dashboard",
				Usage: "Talk to Dashboard API and manage your devices and groups without using web frontend.",
				Subcommands: []*cli.Command{
					{
						Name:   "login",
						Usage:  "obtain short-lived API token to authenticate while executing queries",
						Action: handleLogin,
					},
					{
						Name:     "group",
						Usage:    "Husarnet group management, eg. see your groups, create, rename, delete",
						Category: "dashboard",
						Subcommands: []*cli.Command{
							{
								Name:    "list",
								Aliases: []string{"ls"},
								Usage:   "display a table of all your groups (with summary information)",
								Action:  handleGroupLs,
							},
							{
								Name:   "show",
								Usage:  "display a table of devices in a given group",
								Action: handleGroupShow,
							},
							{
								Name:   "unjoin",
								Usage:  "remove given device from the given group. First arg is group ID and second is the fragment of device IPv6",
								Action: handleGroupUnjoin,
							},
							{
								Name:    "create",
								Aliases: []string{"add"},
								Usage:   "create new group with a name given as an argument",
								Action:  handleGroupCreate,
							},
							{
								Name:   "rename",
								Usage:  "change name for group with id [ID] to [new name]",
								Action: handleGroupRename,
							},
							{
								Name:    "remove",
								Aliases: []string{"rm"},
								Usage:   "remove the group with given ID. Will ask for confirmation.",
								Action:  handleGroupRemove,
							},
						},
					},
					{
						Name:     "device",
						Usage:    "Husarnet device management",
						Category: "dashboard",
						Subcommands: []*cli.Command{
							{
								Name:    "list",
								Aliases: []string{"ls"},
								Usage:   "display a table of all your devices and which groups are they in",
								Action:  handleDeviceLs,
							},
							{
								Name:   "rename",
								Usage:  "change displayed name of a device",
								Action: handleDeviceRename,
							},
							{
								Name:    "remove",
								Aliases: []string{"rm"},
								Usage:   "remove device from your account",
								Action:  handleDeviceRemove},
						},
					},
				},
			},
		},
	}

	err := app.Run(os.Args)
	if err != nil {
		log.Fatal(err)
	}
}
