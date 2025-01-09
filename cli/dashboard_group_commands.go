// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"github.com/husarnet/husarnet/cli/v2/output"
	"github.com/husarnet/husarnet/cli/v2/requests"
	"github.com/husarnet/husarnet/cli/v2/types"

	"github.com/urfave/cli/v3"
)

var dashboardGroupListCommand = &cli.Command{
	Name:      "list",
	Aliases:   []string{"ls"},
	Usage:     "Display a table of all your groups (with summary information)",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx context.Context, cmd *cli.Command) error {
		ignoreExtraArguments(cmd)

		resp, err := requests.FetchGroups()
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			output.PrintJsonOrError(resp)
		} else {
			prettyPrintGroups(resp.Payload)
		}
		return nil
	},
}

var dashboardGroupShowCommand = &cli.Command{
	Name:      "show",
	Usage:     "Display group details",
	ArgsUsage: "<group name or id>",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)

		uuid, err := determineGroupUuid(cmd.Args().First())
		if err != nil {
			printError(err.Error())
			return nil
		}

		resp, err := requests.FetchGroupByUuid(uuid)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			output.PrintJsonOrError(resp)
		} else {
			prettyPrintGroup(resp.Payload.Group)
		}
		return nil
	},
}

var dashboardGroupCreateCommand = &cli.Command{
	Name:      "create",
	Usage:     "Create a new group",
	ArgsUsage: "<name>",
	Flags: []cli.Flag{
		&cli.StringFlag{
			Name:  "emoji",
			Usage: "you can set custom emoji",
		},
		&cli.StringFlag{
			Name:  "comment",
			Usage: "you can add optional comment to the group",
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)

		params := types.GroupCrudInput{
			Name:    cmd.Args().First(),
			Comment: cmd.String("comment"),
			Emoji:   cmd.String("emoji"),
		}

		resp, err := requests.CreateGroup(params)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			output.PrintJsonOrError(resp)
		} else {
			prettyPrintGroup(resp.Payload)
		}
		return nil
	},
}

var dashboardGroupUpdateCommand = &cli.Command{
	Name:      "update",
	Usage:     "Update group details",
	ArgsUsage: "<group name or id>",
	Flags: []cli.Flag{
		&cli.StringFlag{
			Name:  "name",
			Usage: "you can rename the group",
		},
		&cli.StringFlag{
			Name:  "emoji",
			Usage: "you can set custom emoji",
		},
		&cli.StringFlag{
			Name:  "comment",
			Usage: "you can add optional comment to the group",
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)

		uuid, err := determineGroupUuid(cmd.Args().First())
		if err != nil {
			printError(err.Error())
			return nil
		}

		params := types.GroupCrudInput{
			Name:    cmd.String("name"),
			Comment: cmd.String("comment"),
			Emoji:   cmd.String("emoji"),
		}

		resp, err := requests.UpdateGroup(uuid, params)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			output.PrintJsonOrError(resp)
		} else {
			prettyPrintGroup(resp.Payload)
		}
		return nil
	},
}

var dashboardGroupDeleteCommand = &cli.Command{
	Name:      "delete",
	Usage:     "Delete group by ID",
	ArgsUsage: "<group name or id>",
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:  "yes",
			Usage: "don't ask for confirmation",
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)

		uuid, err := determineGroupUuid(cmd.Args().First())
		if err != nil {
			printError(err.Error())
			return nil
		}

		if !cmd.Bool("yes") {
			askForConfirmation("Are you sure you want to delete this group?")
		}

		resp, err := requests.DeleteGroup(uuid)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			output.PrintJsonOrError(resp)
		} else {
			printSuccess("Group was deleted successfully")
		}
		return nil
	},
}
