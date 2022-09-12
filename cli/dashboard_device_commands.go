// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"hdm/generated"
	"net/netip"
	"strings"

	"github.com/Khan/genqlient/graphql"
	"github.com/pterm/pterm"
	u "github.com/rjNemo/underscore"
	"github.com/urfave/cli/v2"
)

func interactiveGetDeviceIpByName() string {
	response := callDashboardAPI(func(client graphql.Client) (*generated.GetDevicesResponse, error) {
		return generated.GetDevices(client)
	})

	deviceNames := u.Map(response.Devices, func(d generated.GetDevicesDevicesDeviceType) string { return d.Name })

	selectedDevice, _ := pterm.DefaultInteractiveSelect.WithDefaultText("Select device").WithOptions(deviceNames).Show()
	pterm.Info.Printfln("Selected device: %s", pterm.Green(selectedDevice))

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
					newName, _ = pterm.DefaultInteractiveTextInput.WithDefaultText("New device name").Show()
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
			Action: func(c *cli.Context) error {
				// TODO device ip or device name (selector?)
				notImplementedYet() // TODO
				return nil
			}},
	},
}
