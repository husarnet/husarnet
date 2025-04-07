// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"github.com/husarnet/husarnet/cli/v2/output"
	"github.com/husarnet/husarnet/cli/v2/requests"
	"github.com/husarnet/husarnet/cli/v2/types"
	"github.com/husarnet/husarnet/cli/v2/utils"
	"strings"

	"github.com/urfave/cli/v3"
)

var claimCommand = &cli.Command{
	Name:      "claim",
	Usage:     "Assign the device to your Husarnet Dashboard account",
	ArgsUsage: "<claim token or joincode>",
	Aliases:   []string{"join"},
	Flags: []cli.Flag{
		&cli.StringFlag{
			Name:  "hostname",
			Value: utils.GetOwnHostname(),
			Usage: "how you want the device identified in the dashboard. If not provided, will use the hostname as reported by the operating system",
		},
		&cli.StringFlag{
			Name:  "comment",
			Usage: "you can add optional comment to the device",
		},
		&cli.StringFlag{
			Name:  "emoji",
			Usage: "you can add optional emoji to identify the device in the frontend",
		},
		&cli.StringSliceFlag{
			Name:  "alias",
			Usage: "you can add optional aliases (additional hostnames) you can identify this device with",
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
		minimumArguments(cmd, 1)
		args := cmd.Args().Slice()
		token := args[0]

		if strings.HasPrefix(token, "fc94:b01d:1803:8dd8:b293:5c7d:7639:932a/") {
			printWarning("join codes with ipv6 address prefix are deprecated. You can omit the prefix")
		}

		params := types.ClaimParams{
			ClaimToken: args[0],
			Hostname:   cmd.String("hostname"),
			Aliases:    cmd.StringSlice("alias"),
			Comment:    cmd.String("comment"),
			Emoji:      cmd.String("emoji"),
		}

		resp, err := requests.Claim(params)
		if err != nil {
			printError(err.Error())
			return nil
		}

		if rawJson {
			output.PrintJsonOrError(resp)
			return nil
		}

		output.PrintGenericApiResponse(resp, "Claim request was successful", "Claim request failed")
		return nil
		// TODO: optionally add waits, as in the old code
	},
}
