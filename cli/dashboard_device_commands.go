// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"slices"

	"github.com/urfave/cli/v3"
)

var dashboardDeviceListCommand = &cli.Command{
	Name:      "list",
	Aliases:   []string{"ls"},
	Usage:     "Display a table of all your devices",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx context.Context, cmd *cli.Command) error {
		ignoreExtraArguments(cmd)

		resp, err := fetchDevices()
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			printJsonOrError(resp)
			return nil
		}

		prettyPrintDevices(resp)
		return nil
	},
}

var dashboardDeviceShowCommand = &cli.Command{
	Name:      "show",
	Usage:     "Display device details",
	ArgsUsage: "<device uuid OR Husarnet IPv6 addr OR hostname>",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)
		arg := cmd.Args().First()
		uuid, err := determineDeviceUuid(arg)
		if err != nil {
			printError(err.Error())
			return nil
		}

		resp, err := fetchDeviceByUuid(uuid)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			printJsonOrError(resp)
		} else {
			prettyPrintDevice(resp)
		}
		return nil
	},
}

var dashboardDeviceUpdateCommand = &cli.Command{
	Name:      "update",
	Usage:     "Update device details",
	ArgsUsage: "<device uuid OR Husarnet IPv6 addr OR hostname>",
	Flags: []cli.Flag{
		&cli.StringFlag{
			Name:  "hostname",
			Usage: "you can change primary hostname",
		},
		&cli.StringFlag{
			Name:  "emoji",
			Usage: "you can set custom emoji",
		},
		&cli.StringFlag{
			Name:  "comment",
			Usage: "you can add optional comment to the device",
		},
		&cli.StringSliceFlag{
			Name:  "add-alias",
			Usage: "you can add new hostname aliases",
		},
		&cli.StringSliceFlag{
			Name:  "remove-alias",
			Usage: "you can remove hostname aliases",
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsRange(cmd, 0, 1)
		arg := cmd.Args().First() // will be empty string if not provided
		if arg == "" {
			// updating self. we can use our own IP as a parameter
			status := getDaemonStatus()
			// TODO: error here if we can't contact daemon
			arg = status.LocalIP.StringExpanded()
		}

		uuid, err := determineDeviceUuid(arg)
		if err != nil {
			printError(err.Error())
			return nil
		}

		deviceResp, err := fetchDeviceByUuid(uuid)
		if err != nil {
			printError(err.Error())
			return nil
		}

		addAliases := cmd.StringSlice("add-alias")
		removeAliases := cmd.StringSlice("remove-alias")

		aliases := append([]string{}, deviceResp.Payload.Aliases...)
		if len(addAliases) != 0 {
			for _, aliasToAdd := range addAliases {
				if !slices.Contains(aliases, aliasToAdd) {
					aliases = append(aliases, aliasToAdd)
				}
			}
		}
		var updatedAliases []string
		for _, alias := range aliases {
			if !slices.Contains(removeAliases, alias) {
				updatedAliases = append(updatedAliases, alias)
			}
		}

		params := DeviceCrudInput{
			Hostname: cmd.String("hostname"),
			Comment:  cmd.String("comment"),
			Emoji:    cmd.String("emoji"),
			Aliases:  updatedAliases,
		}

		resp, err := reqUpdateDevice(uuid, params)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			printJsonOrError(resp)
		} else {
			prettyPrintDevice(resp)
		}
		return nil
	},
}

var dashboardDeviceUnclaimCommand = &cli.Command{
	Name:      "unclaim",
	Usage:     "Unclaim self (no argument) or some other device (single argument) in your network",
	ArgsUsage: "<device uuid OR Husarnet IPv6 addr OR hostname>",
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:  "yes",
			Usage: "don't ask for confirmation",
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsRange(cmd, 0, 1)

		if cmd.Args().Len() == 0 {
			if !cmd.Bool("yes") {
				askForConfirmation("Are you sure you want to unclaim this device?")
			}
			resp, err := reqUnclaimSelf()
			if err != nil {
				printError(err.Error())
				return nil
			}

			if rawJson {
				printJsonOrError(resp)
			}
			return nil
		}

		uuid, err := determineDeviceUuid(cmd.Args().First())
		if err != nil {
			printError(err.Error())
			return nil
		}

		resp := callDashboardApi[any]("POST", "/web/devices/unclaim/"+uuid)
		if resp.Type != "success" {
			printError("API request failed. Message: %s", resp.Errors[0])
			return nil
		}
		printSuccess("device successfully unclaimed.")
		return nil
	},
}

var dashboardDeviceAttachCommand = &cli.Command{
	Name:      "attach",
	Usage:     "Attach self or another device to a group",
	ArgsUsage: "[device uuid OR Husarnet IPv6 addr OR hostname] <group uuid OR name>",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsRange(cmd, 1, 2)

		// determine inputs
		var paramDevice string
		var paramGroup string
		if cmd.Args().Len() == 2 {
			paramDevice = cmd.Args().Get(0)
			paramGroup = cmd.Args().Get(1)
		} else {
			status := getDaemonStatus()
			// TODO: error here if we can't contact daemon
			paramDevice = status.LocalIP.StringExpanded()
			paramGroup = cmd.Args().First()
		}

		deviceIP, err := determineDeviceIP(paramDevice)
		if err != nil {
			printError(err.Error())
			return nil
		}

		groupUuid, err := determineGroupUuid(paramGroup)
		if err != nil {
			printError(err.Error())
			return nil
		}

		resp := callDashboardApiWithInput[AttachDetachInput, any]("POST", "/web/groups/attach-device", AttachDetachInput{
			GroupId:  groupUuid,
			DeviceIp: deviceIP,
		})
		if resp.Type != "success" {
			printError("API request failed. Message: %s", resp.Errors[0])
			return nil
		}
		printSuccess("device successfully attached")
		return nil
	},
}

var dashboardDeviceDetachCommand = &cli.Command{
	Name:      "detach",
	Usage:     "Detach self or another device from a particular group",
	ArgsUsage: "[device uuid OR Husarnet IPv6 addr OR hostname] <group uuid OR name>",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsRange(cmd, 1, 2)

		// determine inputs
		var paramDevice string
		var paramGroup string
		if cmd.Args().Len() == 2 {
			paramDevice = cmd.Args().Get(0)
			paramGroup = cmd.Args().Get(1)
		} else {
			status := getDaemonStatus()
			// TODO: error here if we can't contact daemon
			paramDevice = status.LocalIP.StringExpanded()
			paramGroup = cmd.Args().First()
		}

		deviceIP, err := determineDeviceIP(paramDevice)
		if err != nil {
			printError(err.Error())
			return nil
		}

		groupUuid, err := determineGroupUuid(paramGroup)
		if err != nil {
			printError(err.Error())
			return nil
		}

		resp := callDashboardApiWithInput[AttachDetachInput, any]("POST", "/web/groups/detach-device", AttachDetachInput{
			GroupId:  groupUuid,
			DeviceIp: deviceIP,
		})
		if resp.Type != "success" {
			printError("API request failed. Message: %s", resp.Errors[0])
			return nil
		}
		printSuccess("device successfully detached")
		return nil
	},
}
