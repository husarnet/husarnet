// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"hdm/dashboard_handlers"

	"github.com/urfave/cli/v2"
)

var dashboardDeviceCommand = &cli.Command{
	Name:     "device",
	Usage:    "Husarnet device management",
	Category: "dashboard",
	Subcommands: []*cli.Command{
		{
			Name:    "list",
			Aliases: []string{"ls"},
			Usage:   "display a table of all your devices and which groups are they in",
			Action: func(c *cli.Context) error {
				callAPI(dashboard_handlers.ListDevicesHandler{})
				return nil
			},
		},
		{
			Name:  "rename",
			Usage: "change displayed name of a device",
			Action: func(c *cli.Context) error {
				if c.Args().Len() < 2 {
					fmt.Println(notEnoughArgsForRenameDevice)
					return nil
				}
				callAPI(dashboard_handlers.RenameDeviceHandler{}, c.Args().Get(0), c.Args().Get(1))
				return nil
			},
		},
		{
			Name:    "remove",
			Aliases: []string{"rm"},
			Usage:   "remove device from your account",
			Action: func(c *cli.Context) error {
				notImplementedYet() // TODO
				return nil
			}},
	},
}
