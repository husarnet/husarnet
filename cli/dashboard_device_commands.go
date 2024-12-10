// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"fmt"
	"github.com/urfave/cli/v3"
)

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
	Usage:     "unclaim self (no argument) or some other device (single argument) in your network",
	ArgsUsage: "<device uuid OR Husarnet IPv6 addr OR hostname>",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsRange(cmd, 0, 1)

		if cmd.Args().Len() == 0 {
			// unclaiming self, use device manage route
			resp := callDashboardApi[any]("POST", "/device/manage/unclaim")
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
		}

		// here we get the device ip, id or hostname/alias, we will need to query the api
		arg := cmd.Args().First()
		if looksLikeUuidv4(arg) {
			// we can try unclaiming right away
			resp := callDashboardApi[any]("GET", "/device/manage/unclaim")
			if resp.Type != "success" {
				printError("API request failed. Message: %s", resp.Errors[0])
				return nil
			}
			return nil
		}

		if looksLikeIpv6(arg) {
			// try a roundtrip

			return nil
		}

		return nil
	},
}
