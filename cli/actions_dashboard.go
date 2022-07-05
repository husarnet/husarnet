// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"hdm/handlers"

	"github.com/urfave/cli/v2"
)

func handleLogin(c *cli.Context) error {
	loginAndSaveAuthToken()
	return nil
}

func handleGroupLs(c *cli.Context) error {
	callAPI(handlers.ListGroupsHandler{})
	return nil
}

func handleGroupShow(c *cli.Context) error {
	if c.Args().Len() < 1 {
		fmt.Println(notEnoughArgsForShowGroup)
		return nil
	}
	callAPI(handlers.ShowGroupHandler{}, c.Args().First())
	return nil
}

func handleGroupUnjoin(c *cli.Context) error {
	if c.Args().Len() < 2 {
		fmt.Println(notEnoughArgsForUnjoin)
		return nil
	}
	callAPI(handlers.UnjoinDeviceHandler{}, c.Args().Get(0), c.Args().Get(1))
	return nil
}

func handleGroupCreate(c *cli.Context) error {
	if c.Args().Len() < 1 {
		fmt.Println(notEnoughArgsForCreateGroup)
		return nil
	}
	callAPI(handlers.CreateGroupHandler{}, c.Args().First())
	return nil
}

func handleGroupRename(c *cli.Context) error {
	if c.Args().Len() < 2 {
		fmt.Println(notEnoughArgsForRenameGroup)
		return nil
	}
	callAPI(handlers.RenameGroupHandler{}, c.Args().Get(0), c.Args().Get(1))
	return nil
}

func handleGroupRemove(c *cli.Context) error {
	if c.Args().Len() < 1 {
		fmt.Println(notEnoughArgsForRemoveGroup)
		return nil
	}
	askForConfirmation(removeGroupConfirmationPrompt)
	callAPI(handlers.RemoveGroupHandler{}, c.Args().First())
	return nil
}

func handleDeviceLs(c *cli.Context) error {
	callAPI(handlers.ListDevicesHandler{})
	return nil
}

func handleDeviceRename(c *cli.Context) error {
	if c.Args().Len() < 2 {
		fmt.Println(notEnoughArgsForRenameDevice)
		return nil
	}
	callAPI(handlers.RenameDeviceHandler{}, c.Args().Get(0), c.Args().Get(1))
	return nil
}

func handleDeviceRemove(c *cli.Context) error {
	notImplementedYet()
	return nil
}
