// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"strings"

	"github.com/husarnet/husarnet/cli/v2/generated"

	"github.com/Khan/genqlient/graphql"
	"github.com/pterm/pterm"
	u "github.com/rjNemo/underscore"
	"github.com/urfave/cli/v2"
)

var dashboardLoginCommand = &cli.Command{
	Name:      "login",
	Usage:     "obtain short-lived API token to authenticate while executing queries",
	ArgsUsage: "[username] [password]",
	Action: func(c *cli.Context) error {
		requiredArgumentsRange(c, 0, 2)

		var username, password string

		if c.Args().Len() == 2 {
			username = c.Args().Get(0)
			password = c.Args().Get(1)
		} else if c.Args().Len() == 1 {
			username = c.Args().Get(0)
			password = getPasswordFromStandardInput()
		} else {
			username, password = getUserCredentialsFromStandardInput()
		}

		loginAndSaveAuthToken(username, password)
		return nil
	},
}

func getDeviceIdAndGroupIdFromUser(ctx *cli.Context) (string, string) {
	requiredArgumentsRange(ctx, 0, 2)

	var deviceId, groupId string

	if ctx.Args().Len() > 0 {
		deviceId = getDeviceIpByNameOrIp(ctx.Args().Get(0))
	} else {
		deviceId = interactiveGetDeviceIpByName()
	}

	if ctx.Args().Len() > 1 {
		groupId = getGroupIdByNameOrId(ctx.Args().Get(1))
	} else {
		groupId = interactiveGetGroupByName().Id
	}

	return deviceId, groupId
}

var dashboardCommand = &cli.Command{
	Name:  "dashboard",
	Usage: "Talk to Dashboard API and manage your devices and groups without using web frontend.",
	Subcommands: []*cli.Command{
		dashboardLoginCommand,

		dashboardGroupCommand,
		dashboardDeviceCommand,

		{
			Name:      "assign",
			Aliases:   []string{"add", "append", "push"}, // Don't get too attached to those aliases - some will go :P
			Usage:     "assign device to a group",
			ArgsUsage: "[device ip or name] [group id or name]",
			Action: func(ctx *cli.Context) error {
				deviceId, groupId := getDeviceIdAndGroupIdFromUser(ctx)

				response := callDashboardAPI(func(client graphql.Client) (*generated.AssignDeviceGroupResponse, error) {
					return generated.AssignDeviceGroup(client, deviceId, groupId)
				})

				pterm.Printfln("Current list of groups for device %s: %s", response.UpdateDevice.Device.Name, strings.Join(u.Map(response.UpdateDevice.Device.GroupMemberships, func(m generated.AssignDeviceGroupUpdateDeviceUpdateDeviceMutationDeviceDeviceTypeGroupMembershipsGroupType) string {
					return m.Name
				}), ", "))

				return nil
			},
		},
		{
			Name:      "unassign",
			Aliases:   []string{"deassign", "rm", "remove", "pop"}, // Don't get too attached to those aliases - some will go :P
			Usage:     "remove device from a group",
			ArgsUsage: "[device ip or name] [group id or name]",
			Action: func(ctx *cli.Context) error {
				deviceId, groupId := getDeviceIdAndGroupIdFromUser(ctx)

				response := callDashboardAPI(func(client graphql.Client) (*generated.UnassignDeviceGroupResponse, error) {
					return generated.UnassignDeviceGroup(client, deviceId, groupId)
				})

				pterm.Printfln("Current list of groups for device %s: %s", response.UpdateDevice.Device.Name, strings.Join(u.Map(response.UpdateDevice.Device.GroupMemberships, func(m generated.UnassignDeviceGroupUpdateDeviceUpdateDeviceMutationDeviceDeviceTypeGroupMembershipsGroupType) string {
					return m.Name
				}), ", "))

				return nil
			},
		},
	},
}
