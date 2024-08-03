// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"encoding/json"
	"fmt"
	"github.com/urfave/cli/v2"
	"strings"
)

type ClaimParams struct {
	ClaimToken string   `json:"token" binding:"required"`
	Hostname   string   `json:"hostname,omitempty"`
	Aliases    []string `json:"aliases,omitempty"`
	Comment    string   `json:"comment,omitempty"`
	Emoji      string   `json:"emoji,omitempty"`
}

var claimCommand = &cli.Command{
	Name:      "claim",
	Usage:     "Assign the device to your Husarnet Dashboard account",
	ArgsUsage: "token",
	Aliases:   []string{"join"},
	Flags: []cli.Flag{
		&cli.StringFlag{
			Name:  "hostname",
			Value: getOwnHostname(),
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
	Action: func(ctx *cli.Context) error {
		minimumArguments(ctx, 1)
		args := ctx.Args().Slice()
		token := args[0]

		if strings.HasPrefix(token, "fc94:b01d:1803:8dd8:b293:5c7d:7639:932a/") {
			die("Provided claim token is old join code format, which is discontinued. Go to dashboard.husarnet.com to obtain a new one.")
		}

		params := ClaimParams{
			ClaimToken: args[0],
			Hostname:   ctx.String("hostname"),
			Aliases:    ctx.StringSlice("alias"),
			Comment:    ctx.String("comment"),
			Emoji:      ctx.String("emoji"),
		}

		jsonBytes, err := json.Marshal(params)
		if err != nil {
			dieE(err)
		}
		fmt.Println(string(jsonBytes))
		resp := callDashboardApiWithInput[ClaimParams, any]("POST", "/device/manage/claim", params)
		if resp.Type == "success" {
			printSuccess("Claim request was successful")
			if len(resp.Warnings) > 0 {
				for _, warning := range resp.Warnings {
					printWarning(warning)
				}
			}
		} else {
			printError("API request failed. Message: %s", resp.Errors[0])
		}

		// TODO: optionally add waits, as in the old code

		return nil
	},
}
