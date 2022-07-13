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

var dashboardCommand = &cli.Command{
	Name:  "dashboard",
	Usage: "Talk to Dashboard API and manage your devices and groups without using web frontend.",
	Subcommands: []*cli.Command{
		dashboardLoginCommand,
		dashboardGroupCommand,
		dashboardDeviceCommand,
	},
}
