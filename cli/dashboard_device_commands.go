// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"hdm/generated"
	"strings"

	"github.com/Khan/genqlient/graphql"
	u "github.com/rjNemo/underscore"
	"github.com/urfave/cli/v2"
)

var dashboardDeviceCommand = &cli.Command{
	Name:     "device",
	Usage:    "Husarnet device management",
	Category: "dashboard",
	Subcommands: []*cli.Command{
		{
			Name:      "list",
			Aliases:   []string{"ls"},
			Usage:     "display a table of all your devices and which groups are they in",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				ignoreExtraArguments(ctx)

				response := callDashboardAPI(func(client graphql.Client) (*generated.GetDevicesResponse, error) {
					return generated.GetDevices(client)
				})

				table := Table{}
				table.SetTitle("Your devices:")
				table.SetHeader("ID(ipv6)", "Name", "Version", "UserAgent", "Groups containing device")

				for _, device := range response.Devices {
					groups := strings.Join(u.Map(device.GroupMemberships, func(membership generated.GetDevicesDevicesDeviceTypeGroupMembershipsGroupType) string {
						return membership.Name
					}), ", ")
					table.AddRow(device.DeviceId, device.Name, device.Version, strings.TrimSpace(device.UserAgent), groups)
				}

				table.Println()

				return nil
			},
		},
		{
			Name:      "rename",
			Usage:     "change displayed name of a device",
			ArgsUsage: "[device ip] [new name]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsNumber(ctx, 2)

				callDashboardAPI(func(client graphql.Client) (*generated.RenameDeviceResponse, error) {
					return generated.RenameDevice(client, ctx.Args().Get(0), ctx.Args().Get(1))
				})

				printSuccess("Device renamed successfully.")

				return nil
			},
		},
		{
			Name:      "remove",
			Aliases:   []string{"rm"},
			Usage:     "remove device from your account",
			ArgsUsage: "[device ip]",
			Action: func(c *cli.Context) error {
				notImplementedYet() // TODO
				return nil
			}},
	},
}
