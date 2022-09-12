// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"github.com/urfave/cli/v2"
)

var dashboardLoginCommand = &cli.Command{
	Name:  "login",
	Usage: "obtain short-lived API token to authenticate while executing queries",
	Action: func(c *cli.Context) error {
		loginAndSaveAuthToken()
		return nil
	},
}

// TODO implement this
// func getDeviceIdAndGroupIdFromUser(ctx *cli.Context) (string, string) {
// 	requiredArgumentsRange(ctx, 0, 2)

// 	var deviceId, groupId string

// 	if ctx.Args().Len() > 0 {
// 		deviceId = getDeviceIpByNameOrIp(ctx.Args().Get(0))
// 	} else {
// 		deviceId = interactiveGetDeviceIpByName()
// 	}

// 	if ctx.Args().Len() > 1 {
// 		groupId = getGroupIdByNameOrId(ctx.Args().Get(1))
// 	} else {
// 		groupId = interactiveGetGroupByName().Id
// 	}

// 	return deviceId, groupId
// }

var dashboardCommand = &cli.Command{
	Name:  "dashboard",
	Usage: "Talk to Dashboard API and manage your devices and groups without using web frontend.",
	Subcommands: []*cli.Command{
		dashboardLoginCommand,

		dashboardGroupCommand,
		dashboardDeviceCommand,

		{
			Name:      "assign",
			Aliases:   []string{"add", "append", "push"}, // Don't get too attached to those aliases - some will go :P
			Usage:     "assign device to a group",
			ArgsUsage: "[device ip or name] [group id or name]",
			Action: func(ctx *cli.Context) error {
				// deviceId, groupId := getDeviceIdAndGroupIdFromUser(ctx)

				notImplementedYet() // TODO implement this

				return nil
			},
		},
		{
			Name:      "unassign",
			Aliases:   []string{"deassign", "remove", "pop"}, // Don't get too attached to those aliases - some will go :P
			Usage:     "remove device from a group",
			ArgsUsage: "[device ip or name] [group id or name]",
			Action: func(ctx *cli.Context) error {
				// deviceId, groupId := getDeviceIdAndGroupIdFromUser(ctx)

				notImplementedYet() // TODO implement this

				return nil
			},
		},
	},
}
