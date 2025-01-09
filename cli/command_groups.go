// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import "github.com/urfave/cli/v3"

var deviceCommands = &cli.Command{
	Name:     "device",
	Aliases:  []string{"devices"},
	Usage:    "Device management, eg. list devices claimed to your account, update, unclaim (authorized devices only)",
	Category: CategoryApi,
	Commands: []*cli.Command{
		dashboardDeviceListCommand,
		dashboardDeviceShowCommand,
		dashboardDeviceUpdateCommand,
		dashboardDeviceUnclaimCommand,
		dashboardDeviceAttachCommand,
		dashboardDeviceDetachCommand,
	},
}

var groupCommands = &cli.Command{
	Name:     "group",
	Aliases:  []string{"groups"},
	Usage:    "Group management, eg. list groups created in your account, add new, update, delete (authorized devices only)",
	Category: CategoryApi,
	Commands: []*cli.Command{
		dashboardGroupListCommand,
		dashboardGroupShowCommand,
		dashboardGroupCreateCommand,
		dashboardGroupUpdateCommand,
		dashboardGroupDeleteCommand,
	},
}
