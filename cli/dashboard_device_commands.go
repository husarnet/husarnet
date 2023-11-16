// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"net/netip"
	"strings"

	"github.com/husarnet/husarnet/cli/v2/generated"

	"github.com/Khan/genqlient/graphql"
	u "github.com/rjNemo/underscore"
	"github.com/urfave/cli/v2"
)

func interactiveGetDeviceIpByName() string {
	response := callDashboardAPI(func(client graphql.Client) (*generated.GetDevicesResponse, error) {
		return generated.GetDevices(client)
	})

	deviceNames := u.Map(response.Devices, func(d generated.GetDevicesDevicesDeviceType) string { return d.Name })

	selectedDevice := interactiveChooseFrom("Select device", deviceNames)

	device, err := u.Find(response.Devices, func(d generated.GetDevicesDevicesDeviceType) bool { return d.Name == selectedDevice })
	if err != nil {
		dieE(err)
	}

	return device.DeviceId
}

func getDeviceIpByName(name string) string {
	response := callDashboardAPI(func(client graphql.Client) (*generated.GetDevicesResponse, error) {
		return generated.GetDevices(client)
	})

	device, err := u.Find(response.Devices, func(d generated.GetDevicesDevicesDeviceType) bool { return d.Name == name })
	if err != nil {
		dieE(err)
	}

	return device.DeviceId
}

func getDeviceIpByNameOrIp(candidate string) string {
	addr, err := netip.ParseAddr(candidate)
	if err == nil {
		return addr.String()
	}
	return getDeviceIpByName(candidate)
}

var dashboardDeviceCommand = &cli.Command{
	Name:     "device",
	Aliases:  []string{"devices"},
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
				table.SetTitle("Your devices")
				table.SetHeader("ID(ipv6)", "Name", "Version", "UserAgent", "Groups containing device")

				for _, device := range response.Devices {
					groups := strings.Join(u.Map(device.GroupMemberships, func(membership generated.GetDevicesDevicesDeviceTypeGroupMembershipsGroupType) string {
						return membership.Name
					}), ", ")
					table.AddRow(makeCannonicalAddr(device.DeviceId), device.Name, device.Version, strings.TrimSpace(device.UserAgent), groups)
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
				requiredArgumentsRange(ctx, 0, 2)

				var deviceIp string

				if ctx.Args().Len() < 1 {
					deviceIp = interactiveGetDeviceIpByName()
				} else {
					deviceIp = getDeviceIpByNameOrIp(ctx.Args().Get(0))
				}

				var newName string

				if ctx.Args().Len() < 2 {
					newName = interactiveTextInput("New device name")
				} else {
					newName = ctx.Args().Get(1)
				}

				callDashboardAPI(func(client graphql.Client) (*generated.RenameDeviceResponse, error) {
					return generated.RenameDevice(client, deviceIp, newName)
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
			Action: func(ctx *cli.Context) error {
				requiredArgumentsRange(ctx, 0, 1)

				var deviceIp string

				if ctx.Args().Len() < 1 {
					deviceIp = interactiveGetDeviceIpByName()
				} else {
					deviceIp = getDeviceIpByNameOrIp(ctx.Args().Get(0))
				}

				callDashboardAPI(func(client graphql.Client) (*generated.RemoveDeviceResponse, error) {
					return generated.RemoveDevice(client, deviceIp)
				})

				printSuccess("Device removed successfully.")

				return nil
			}},
	},
}
