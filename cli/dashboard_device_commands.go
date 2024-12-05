// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"fmt"
	"github.com/urfave/cli/v3"
	"time"
)

type Device struct {
	Id          string    `json:"id"`
	Ip          string    `json:"ip"`
	Hostname    string    `json:"hostname"`
	Comment     string    `json:"comment"`
	LastContact time.Time `json:"lastContact"`
	Version     string    `json:"version"`
	UserAgent   string    `json:"userAgent"`
}

type Devices []Device

var dashboardDeviceCommand = &cli.Command{
	Name:  "device",
	Usage: "Husarnet device management, eg. list devices claimed to your account, update, unclaim",
	Commands: []*cli.Command{
		dashboardDeviceListCommand,
		dashboardDeviceShowCommand,
	},
}

var dashboardDeviceListCommand = &cli.Command{
	Name:      "list",
	Aliases:   []string{"ls"},
	Usage:     "display a table of all your devices",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx context.Context, cmd *cli.Command) error {
		ignoreExtraArguments(cmd)
		resp := callDashboardApi[Devices]("GET", "/web/devices")
		if resp.Type != "success" {
			printError("API request failed. Message: %s", resp.Errors[0])
			return nil
		}

		if !rawJson {
			fmt.Println("pretty print not yet implemented, returning json anyway")
			rawJson = true
		}

		if rawJson {
			printJsonOrError(resp)
		}
		return nil
	},
}

var dashboardDeviceShowCommand = &cli.Command{
	Name:      "show",
	Usage:     "display device details",
	ArgsUsage: "<device id>", // No arguments needed
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)
		resp := callDashboardApi[Device]("GET", "/web/devices/"+cmd.Args().First())
		if resp.Type != "success" {
			printError("API request failed. Message: %s", resp.Errors[0])
			return nil
		}

		if !rawJson {
			fmt.Println("pretty print not yet implemented, returning json anyway")
			rawJson = true
		}

		if rawJson {
			printJsonOrError(resp)
		}
		return nil
	},
}

var dashboardDeviceUnclaimCommand = &cli.Command{
	Name:      "unclaim",
	Usage:     "unclaim self or some other device in your network",
	ArgsUsage: "<device id>", // No arguments needed
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsRange(cmd, 0, 1)
		resp := callDashboardApi[Device]("GET", "/web/devices/"+cmd.Args().First())
		if resp.Type != "success" {
			printError("API request failed. Message: %s", resp.Errors[0])
			return nil
		}

		if !rawJson {
			fmt.Println("pretty print not yet implemented, returning json anyway")
			rawJson = true
		}

		if rawJson {
			printJsonOrError(resp)
		}
		return nil
	},
}
