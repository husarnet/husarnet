// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"github.com/urfave/cli/v3"
)

var dashboardGroupListCommand = &cli.Command{
	Name:      "list",
	Aliases:   []string{"ls"},
	Usage:     "display a table of all your groups (with summary information)",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx context.Context, cmd *cli.Command) error {
		ignoreExtraArguments(cmd)

		resp, err := fetchGroups()
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			printJsonOrError(resp)
		} else {
			prettyPrintGroups(resp)
		}
		return nil
	},
}

var dashboardGroupShowCommand = &cli.Command{
	Name:      "show",
	Usage:     "display group details",
	ArgsUsage: "<group name or id>",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)

		uuid, err := determineGroupUuid(cmd.Args().First())
		if err != nil {
			printError(err.Error())
			return nil
		}

		resp, err := fetchGroupByUuid(uuid)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			printJsonOrError(resp)
		} else {
			prettyPrintGroupDetails(resp)
		}
		return nil
	},
}

var dashboardGroupCreateCommand = &cli.Command{
	Name:      "create",
	Usage:     "create a new group",
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

		params := GroupCrudInput{
			Name:    cmd.Args().First(),
			Comment: cmd.String("comment"),
			Emoji:   cmd.String("emoji"),
		}

		resp, err := reqCreateGroup(params)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			printJsonOrError(resp)
		} else {
			prettyPrintGroup(resp)
		}
		return nil
	},
}

var dashboardGroupUpdateCommand = &cli.Command{
	Name:      "update",
	Usage:     "update group details",
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

		params := GroupCrudInput{
			Name:    cmd.String("name"),
			Comment: cmd.String("comment"),
			Emoji:   cmd.String("emoji"),
		}

		resp, err := reqUpdateGroup(uuid, params)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			printJsonOrError(resp)
		} else {
			prettyPrintGroup(resp)
		}
		return nil
	},
}

var dashboardGroupDeleteCommand = &cli.Command{
	Name:      "delete",
	Usage:     "delete group by ID",
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

		resp, err := reqDeleteGroup(uuid)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			printJsonOrError(resp)
		} else {
			printSuccess("Group was deleted successfully")
		}
		return nil
	},
}
