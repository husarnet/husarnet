package main

import "github.com/urfave/cli/v3"

var deviceCommands = &cli.Command{
	Name:  "device",
	Usage: "Husarnet device management, eg. list devices claimed to your account, update, unclaim",
	Commands: []*cli.Command{
		dashboardDeviceListCommand,
		dashboardDeviceShowCommand,
	},
}

var groupCommands = &cli.Command{
	Name:    "group",
	Aliases: []string{"groups"},
	Usage:   "Husarnet group management, eg. see your groups, create, update, delete",
	Commands: []*cli.Command{
		dashboardGroupListCommand,
		dashboardGroupShowCommand,
		dashboardGroupCreateCommand,
		dashboardGroupUpdateCommand,
		dashboardGroupDeleteCommand,
	},
}
