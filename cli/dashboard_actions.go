// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"hdm/dashboard_handlers"

	"github.com/urfave/cli/v2"
)

func handleLogin(c *cli.Context) error {
	loginAndSaveAuthToken()
	return nil
}

func handleGroupLs(c *cli.Context) error {
	callAPI(dashboard_handlers.ListGroupsHandler{})
	return nil
}

// TODO this should be able to get the group by name
func handleGroupShow(c *cli.Context) error {
	if c.Args().Len() < 1 {
		fmt.Println(notEnoughArgsForShowGroup)
		return nil
	}
	callAPI(dashboard_handlers.ShowGroupHandler{}, c.Args().First())
	return nil
}

func handleGroupUnjoin(c *cli.Context) error {
	if c.Args().Len() < 2 {
		fmt.Println(notEnoughArgsForUnjoin)
		return nil
	}
	callAPI(dashboard_handlers.UnjoinDeviceHandler{}, c.Args().Get(0), c.Args().Get(1))
	return nil
}

func handleGroupCreate(c *cli.Context) error {
	if c.Args().Len() < 1 {
		fmt.Println(notEnoughArgsForCreateGroup)
		return nil
	}
	callAPI(dashboard_handlers.CreateGroupHandler{}, c.Args().First())
	return nil
}

func handleGroupRename(c *cli.Context) error {
	if c.Args().Len() < 2 {
		fmt.Println(notEnoughArgsForRenameGroup)
		return nil
	}
	callAPI(dashboard_handlers.RenameGroupHandler{}, c.Args().Get(0), c.Args().Get(1))
	return nil
}

func handleGroupRemove(c *cli.Context) error {
	if c.Args().Len() < 1 {
		fmt.Println(notEnoughArgsForRemoveGroup)
		return nil
	}
	askForConfirmation(removeGroupConfirmationPrompt)
	callAPI(dashboard_handlers.RemoveGroupHandler{}, c.Args().First())
	return nil
}

func handleDeviceLs(c *cli.Context) error {
	callAPI(dashboard_handlers.ListDevicesHandler{})
	return nil
}

func handleDeviceRename(c *cli.Context) error {
	if c.Args().Len() < 2 {
		fmt.Println(notEnoughArgsForRenameDevice)
		return nil
	}
	callAPI(dashboard_handlers.RenameDeviceHandler{}, c.Args().Get(0), c.Args().Get(1))
	return nil
}

func handleDeviceRemove(c *cli.Context) error {
	notImplementedYet()
	return nil
}
