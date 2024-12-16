package main

import "github.com/urfave/cli/v3"

var deviceCommands = &cli.Command{
	Name:    "device",
	Aliases: []string{"devices"},
	Usage:   "(authorized only) device management, eg. list devices claimed to your account, update, unclaim",
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
	Name:    "group",
	Aliases: []string{"groups"},
	Usage:   "(authorized only) group management, eg. list groups created in your account, add new, update, delete",
	Commands: []*cli.Command{
		dashboardGroupListCommand,
		dashboardGroupShowCommand,
		dashboardGroupCreateCommand,
		dashboardGroupUpdateCommand,
		dashboardGroupDeleteCommand,
	},
}
