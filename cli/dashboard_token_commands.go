// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"github.com/urfave/cli/v3"
)

type UserSettings struct {
	ClaimToken string `json:"claimToken"`
}

type UserResponse struct {
	Settings UserSettings `json:"settings"`
}

var dashboardTokenCommand = &cli.Command{
	Name:  "token",
	Usage: "print or rotate claim token associated with your account",
	Commands: []*cli.Command{
		dashboardTokenPrintCommand,
		dashboardTokenRotateCommand,
	},
}

var dashboardTokenPrintCommand = &cli.Command{
	Name:      "print",
	Usage:     "print your claim token",
	ArgsUsage: "",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		resp := callDashboardApi[UserResponse]("GET", "/web/user")
		if resp.Type == "success" {
			printSuccess("User request was successful")
			printInfo(resp.Payload.Settings.ClaimToken)
		} else {
			printError("API request failed. Message: %s", resp.Errors[0])
		}
		return nil
	},
}

var dashboardTokenRotateCommand = &cli.Command{
	Name:      "rotate",
	Usage:     "rotate your claim token (you can do it once a minute)",
	ArgsUsage: "",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		resp := callDashboardApi[UserResponse]("POST", "/web/settings/rotate-claim-token")
		if resp.Type == "success" {
			printSuccess("User request was successful")
			printInfo(resp.Payload.Settings.ClaimToken) // TODO fix
		} else {
			printError("API request failed. Message: %s", resp.Errors[0])
		}
		return nil
	},
}
