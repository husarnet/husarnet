// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"fmt"
	"github.com/urfave/cli/v3"
)

type Group struct {
	Id      string `json:"id"`
	Emoji   string `json:"emoji"`
	Name    string `json:"name"`
	Comment string `json:"comment"`
}

type JoinCode struct {
	Token string `json:"token"`
}

type GroupDetails struct {
	Group             Group    `json:"group"`
	AttachableDevices []Device `json:"attachableDevices"`
	JoinCode          JoinCode `json:"joinCode"`
}

type GroupCrudInput struct {
	Name    string `json:"name"`
	Emoji   string `json:"emoji"`
	Comment string `json:"comment"`
}

type Groups []Group

var dashboardGroupCommand = &cli.Command{
	Name:    "group",
	Aliases: []string{"groups"},
	Usage:   "Husarnet group management, eg. see your groups, create, update, delete",
	Commands: []*cli.Command{
		dashboardGroupListCommand,
		dashboardGroupShowCommand,
		dashboardGroupCreateCommand,
		dashboardGroupUpdateCommand,
		dashboardGroupDeleteCommand,
	},
}

var dashboardGroupListCommand = &cli.Command{
	Name:      "list",
	Aliases:   []string{"ls"},
	Usage:     "display a table of all your groups (with summary information)",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx context.Context, cmd *cli.Command) error {
		ignoreExtraArguments(cmd)
		resp := callDashboardApi[Groups]("GET", "/web/groups")
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

var dashboardGroupShowCommand = &cli.Command{
	Name:      "show",
	Usage:     "display group details",
	ArgsUsage: "<group name or id>",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)
		arg := cmd.Args().First()
		uuid := arg
		if !looksLikeUuidv4(arg) {
			// maybe code it as fetchgroupsToContainer() or something...
			// but this if for refactoring stage I guess
			// assume this is group name, make a roundtrip to figure out what it is
			resp := callDashboardApi[Groups]("GET", "/web/groups")
			if resp.Type != "success" {
				printError("API request failed. Message: %s", resp.Errors[0])
				return nil
			}

			groups := resp.Payload
			for _, group := range groups {
				if group.Name == arg {
					uuid = group.Id
					break
				}
			}

			if uuid == arg {
				printError("Group '%s' not found", arg)
				return nil
			}
		}

		resp := callDashboardApi[GroupDetails]("GET", "/web/groups/"+uuid)
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
		resp := callDashboardApiWithInput[GroupCrudInput, Group]("POST", "/web/groups", params)

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

var dashboardGroupUpdateCommand = &cli.Command{
	Name:      "update",
	Usage:     "update group details",
	ArgsUsage: "<group id>",
	Flags: []cli.Flag{
		&cli.StringFlag{
			Name:  "name",
			Usage: "you rename the group",
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

		params := GroupCrudInput{
			Name:    cmd.String("name"),
			Comment: cmd.String("comment"),
			Emoji:   cmd.String("emoji"),
		}
		resp := callDashboardApiWithInput[GroupCrudInput, Group]("PUT", "/web/groups/"+cmd.Args().First(), params)

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

var dashboardGroupDeleteCommand = &cli.Command{
	Name:      "delete",
	Usage:     "delete group by ID",
	ArgsUsage: "<group id>",
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:  "yes",
			Usage: "don't ask for confirmation",
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsNumber(cmd, 1)

		if !cmd.Bool("yes") {
			askForConfirmation("Are you sure you want to delete this group?")
		}

		resp := callDashboardApi[any]("DELETE", "/web/groups/"+cmd.Args().First())

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
