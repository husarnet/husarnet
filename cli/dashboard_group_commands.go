// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"hdm/dashboard_handlers"

	"github.com/urfave/cli/v2"
)

var dashboardGroupCommand = &cli.Command{
	Name:     "group",
	Usage:    "Husarnet group management, eg. see your groups, create, rename, delete",
	Category: "dashboard",
	Subcommands: []*cli.Command{
		{
			Name:    "list",
			Aliases: []string{"ls"},
			Usage:   "display a table of all your groups (with summary information)",
			Action: func(c *cli.Context) error {
				callAPI(dashboard_handlers.ListGroupsHandler{})
				return nil
			},
		},
		{
			Name:  "show",
			Usage: "display a table of devices in a given group",
			Action: func(c *cli.Context) error {
				// TODO this should be able to get the group by name
				if c.Args().Len() < 1 {
					fmt.Println(notEnoughArgsForShowGroup)
					return nil
				}
				callAPI(dashboard_handlers.ShowGroupHandler{}, c.Args().First())
				return nil
			},
		},
		{
			Name:  "unjoin",
			Usage: "remove given device from the given group. First arg is group ID and second is the fragment of device IPv6",
			Action: func(c *cli.Context) error {
				if c.Args().Len() < 2 {
					fmt.Println(notEnoughArgsForUnjoin)
					return nil
				}
				callAPI(dashboard_handlers.UnjoinDeviceHandler{}, c.Args().Get(0), c.Args().Get(1))
				return nil
			},
		},
		{
			Name:    "create",
			Aliases: []string{"add"},
			Usage:   "create new group with a name given as an argument",
			Action: func(c *cli.Context) error {
				if c.Args().Len() < 1 {
					fmt.Println(notEnoughArgsForCreateGroup)
					return nil
				}
				callAPI(dashboard_handlers.CreateGroupHandler{}, c.Args().First())
				return nil
			},
		},
		{
			Name:  "rename",
			Usage: "change name for group with id [ID] to [new name]",
			Action: func(c *cli.Context) error {
				if c.Args().Len() < 2 {
					fmt.Println(notEnoughArgsForRenameGroup)
					return nil
				}
				callAPI(dashboard_handlers.RenameGroupHandler{}, c.Args().Get(0), c.Args().Get(1))
				return nil
			},
		},
		{
			Name:    "remove",
			Aliases: []string{"rm"},
			Usage:   "remove the group with given ID. Will ask for confirmation.",
			Action: func(c *cli.Context) error {
				if c.Args().Len() < 1 {
					fmt.Println(notEnoughArgsForRemoveGroup)
					return nil
				}
				askForConfirmation(removeGroupConfirmationPrompt)
				callAPI(dashboard_handlers.RemoveGroupHandler{}, c.Args().First())
				return nil
			},
		},
	},
}
