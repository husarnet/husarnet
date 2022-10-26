// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"strconv"

	"github.com/husarnet/husarnet/cli/v1/generated"

	"github.com/Khan/genqlient/graphql"
	"github.com/pterm/pterm"
	u "github.com/rjNemo/underscore"
	"github.com/urfave/cli/v2"
)

func interactiveGetGroupByName() generated.GetGroupsGroupsGroupType {
	response := callDashboardAPI(func(client graphql.Client) (*generated.GetGroupsResponse, error) {
		return generated.GetGroups(client)
	})

	groupNames := u.Map(response.Groups, func(g generated.GetGroupsGroupsGroupType) string { return g.Name })

	selectedGroup := interactiveChooseFrom("Select group", groupNames)

	group, err := u.Find(response.Groups, func(g generated.GetGroupsGroupsGroupType) bool { return g.Name == selectedGroup })
	if err != nil {
		dieE(err)
	}

	return group
}

func getGroupByName(name string) generated.GetGroupsGroupsGroupType {
	response := callDashboardAPI(func(client graphql.Client) (*generated.GetGroupsResponse, error) {
		return generated.GetGroups(client)
	})

	group, err := u.Find(response.Groups, func(g generated.GetGroupsGroupsGroupType) bool { return g.Name == name })
	if err != nil {
		dieE(err)
	}

	return group
}

func getGroupIdByNameOrId(candidate string) string {
	_, err := strconv.Atoi(candidate)
	if err == nil {
		return candidate
	}

	return getGroupByName(candidate).Id
}

var dashboardGroupCommand = &cli.Command{
	Name:     "group",
	Aliases:  []string{"groups"},
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
					table.AddRow(group.Id, group.Name, pterm.Sprintf("%d", group.DeviceCount), ShortenJoinCode(group.JoinCode))
				}

				table.Println()

				return nil
			},
		},
		{
			Name:      "show",
			Usage:     "display a table of devices in a given group",
			ArgsUsage: "[group id or name]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsRange(ctx, 0, 1)

				var groupId string // This is a stringified int

				if ctx.Args().Len() < 1 {
					groupId = interactiveGetGroupByName().Id
				} else {
					groupId = getGroupIdByNameOrId(ctx.Args().First())
				}

				response := callDashboardAPI(func(client graphql.Client) (*generated.ShowGroupResponse, error) {
					return generated.ShowGroup(client, groupId)
				})

				table := Table{}
				table.SetTitle("Devices in your group " + pterm.Bold.Sprintf("%s", ctx.Args().First()))
				table.SetHeader("ID(ipv6)", "Name", "Version", "UserAgent")

				for _, member := range response.GroupMembersById {
					table.AddRow(makeCannonicalAddr(member.DeviceId), member.Name, member.Version, member.UserAgent)
				}

				table.Println()

				return nil
			},
		},
		{
			Name:      "create",
			Aliases:   []string{"add"},
			Usage:     "create new group with a name given as an argument",
			ArgsUsage: "[group name]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsRange(ctx, 0, 1)

				var groupName string

				if ctx.Args().Len() == 0 {
					groupName = interactiveTextInput("Group name")
				} else {
					groupName = ctx.Args().First()
				}

				response := callDashboardAPI(func(client graphql.Client) (*generated.CreateGroupResponse, error) {
					return generated.CreateGroup(client, groupName)
				})

				printSuccess(pterm.Sprintf("Group created. New group information:\nID: %s\nName: %s\n", response.CreateGroup.Group.Id, response.CreateGroup.Group.Name))

				return nil
			},
		},
		{
			Name:      "rename",
			Usage:     "change name for group with id [ID] to [new name]",
			ArgsUsage: "[group id or name] [new name]",
			Action: func(ctx *cli.Context) error {
				requiredArgumentsRange(ctx, 0, 2)

				var groupId string // This is a stringified int

				if ctx.Args().Len() < 1 {
					groupId = interactiveGetGroupByName().Id
				} else {
					groupId = getGroupIdByNameOrId(ctx.Args().First())
				}

				var newName string

				if ctx.Args().Len() < 2 {
					newName = interactiveTextInput("New group name")
				} else {
					newName = ctx.Args().Get(1)
				}

				callDashboardAPI(func(client graphql.Client) (*generated.RenameGroupResponse, error) {
					return generated.RenameGroup(client, groupId, newName)
				})

				printSuccess("Group renamed successfully.")

				return nil
			},
		},
		{
			Name:      "remove",
			Aliases:   []string{"rm"},
			Usage:     "remove the group with given ID. Will ask for confirmation.",
			ArgsUsage: "[group id or name]",
			Action: func(ctx *cli.Context) error {
				// TODO think about adding a device argument in case users want to remove devices from the group with this instead of the regular one [group id] (device ip)
				requiredArgumentsRange(ctx, 0, 1)

				var groupId string // This is a stringified int

				if ctx.Args().Len() == 0 {
					groupId = interactiveGetGroupByName().Id
				} else {
					groupId = getGroupIdByNameOrId(ctx.Args().First())
				}

				askForConfirmation("Are you sure you want to delete this group?")

				callDashboardAPI(func(client graphql.Client) (*generated.RemoveGroupResponse, error) {
					return generated.RemoveGroup(client, groupId)
				})

				printSuccess("Group removed successfully.")

				return nil
			},
		},
	},
}
