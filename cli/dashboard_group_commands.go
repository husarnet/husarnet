// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"hdm/generated"

	"github.com/Khan/genqlient/graphql"
	"github.com/pterm/pterm"
	"github.com/urfave/cli/v2"
)

var dashboardGroupCommand = &cli.Command{
	Name:     "group",
	Usage:    "Husarnet group management, eg. see your groups, create, rename, delete",
	Category: "dashboard",
	Subcommands: []*cli.Command{
		{
			Name:      "list",
			Aliases:   []string{"ls"},
			Usage:     "display a table of all your groups (with summary information)",
			ArgsUsage: " ", // No arguments needed
			Action: func(ctx *cli.Context) error {
				ignoreExtraArguments(ctx)

				response := callDashboardAPI(func(client graphql.Client) (*generated.GetGroupsResponse, error) {
					return generated.GetGroups(client)
				})

				table := Table{}
				table.SetTitle("Your groups")
				table.SetHeader("ID", "Name", "DeviceCount", "JoinCode")

				for _, group := range response.Groups {
					table.AddRow(group.Id, group.Name, fmt.Sprintf("%d", group.DeviceCount), ShortenJoinCode(group.JoinCode))
				}

				table.Println()

				return nil
			},
		},
		{
			Name:      "show",
			Usage:     "display a table of devices in a given group",
			ArgsUsage: "[group id]",
			Action: func(ctx *cli.Context) error {
				// TODO this should be able to get the group by name
				requiredArgumentsNumber(ctx, 1)

				response := callDashboardAPI(func(client graphql.Client) (*generated.ShowGroupResponse, error) {
					return generated.ShowGroup(client, ctx.Args().First())
				})

				table := Table{}
				table.SetTitle("Devices in your group " + pterm.Bold.Sprintf("%s", ctx.Args().First()) + ":")
				table.SetHeader("ID(ipv6)", "Name", "Version", "UserAgent")

				for _, member := range response.GroupMembersById {
					table.AddRow(member.DeviceId, member.Name, member.Version, member.UserAgent)
				}

				table.Println()

				return nil
			},
		},
		{
			Name:      "unjoin",
			Usage:     "remove given device from the given group. First arg is group ID and second is the fragment of device IPv6",
			ArgsUsage: "[group id] [device ip]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsNumber(ctx, 2)

				callDashboardAPI(func(client graphql.Client) (*generated.KickDeviceResponse, error) {
					return generated.KickDevice(client, ctx.Args().Get(0), ctx.Args().Get(1))
				})

				printSuccess("Device was successfully removed from the group.")

				return nil
			},
		},
		{
			Name:      "create",
			Aliases:   []string{"add"},
			Usage:     "create new group with a name given as an argument",
			ArgsUsage: "[group name]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsNumber(ctx, 1)

				response := callDashboardAPI(func(client graphql.Client) (*generated.CreateGroupResponse, error) {
					return generated.CreateGroup(client, ctx.Args().First())
				})

				printSuccess(pterm.Sprintf("Group created. New group information:\n\tID: %s\n\tName: %s\n", response.CreateGroup.Group.Id, response.CreateGroup.Group.Name))

				return nil
			},
		},
		{
			Name:      "rename",
			Usage:     "change name for group with id [ID] to [new name]",
			ArgsUsage: "[group id] [new name]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsNumber(ctx, 2)

				callDashboardAPI(func(client graphql.Client) (*generated.RenameGroupResponse, error) {
					return generated.RenameGroup(client, ctx.Args().Get(0), ctx.Args().Get(1))
				})

				printSuccess("Group renamed successfully.")

				return nil
			},
		},
		{
			Name:      "remove",
			Aliases:   []string{"rm"},
			Usage:     "remove the group with given ID. Will ask for confirmation.",
			ArgsUsage: "[group id]",
			Action: func(ctx *cli.Context) error {
				// TODO think about adding a device argument in case users want to remove devices from the group with this instead of the regular one [group id] (device ip)
				requiredArgumentsNumber(ctx, 1)

				askForConfirmation("Are you sure you want to delete this group? All the devices inside the group will be deleted as well.")

				callDashboardAPI(func(client graphql.Client) (*generated.RemoveGroupResponse, error) {
					return generated.RemoveGroup(client, ctx.Args().First())
				})

				printSuccess("Group removed successfully.")

				return nil
			},
		},
	},
}
