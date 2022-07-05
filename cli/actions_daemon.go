// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"net/url"

	"github.com/urfave/cli/v2"
)

func handleVersion(c *cli.Context) error {
	fmt.Println("CLI version: " + version)
	// TODO ask daemon
	return nil
}

func handleStatus(c *cli.Context) error {
	callDaemonGet("/control/status")
	return nil
}

func handleJoin(c *cli.Context) error {
	// up to two params
	if c.Args().Len() < 1 {
		die("you need to provide joincode")
	}
	joincode := c.Args().Get(0)
	hostname := ""

	if c.Args().Len() == 2 {
		hostname = c.Args().Get(1)
	}

	params := url.Values{
		"code":     {joincode},
		"hostname": {hostname},
	}
	addDaemonApiSecret(&params)
	callDaemonPost("/control/join", params)
	return nil
}

func handleSetupServer(c *cli.Context) error {
	if c.Args().Len() < 1 {
		fmt.Println("you need to provide address of the dashboard")
		return nil
	}

	domain := c.Args().Get(0)

	params := url.Values{
		"domain": {domain},
	}
	addDaemonApiSecret(&params)
	callDaemonPost("/control/change-server", params)
	return nil
}

func handleWhitelistLs(c *cli.Context) error {
	callDaemonGet("/control/whitelist/ls")
	return nil
}

func handleWhitelistAdd(c *cli.Context) error {
	if c.Args().Len() < 1 {
		fmt.Println("you need to provide Husarnet address of the device")
		return nil
	}
	addr := c.Args().Get(0)

	params := url.Values{
		"address": {addr},
	}
	addDaemonApiSecret(&params)
	callDaemonPost("/control/whitelist/add", params)
	return nil
}

func handleWhitelistRm(c *cli.Context) error {
	if c.Args().Len() < 1 {
		fmt.Println("you need to provide Husarnet address of the device")
		return nil
	}
	addr := c.Args().Get(0)

	params := url.Values{
		"address": {addr},
	}
	addDaemonApiSecret(&params)
	callDaemonPost("/control/whitelist/rm", params)
	return nil
}
