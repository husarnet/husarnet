// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"github.com/husarnet/husarnet/cli/v2/requests"

	"github.com/urfave/cli/v3"
)

var tokenCommand = &cli.Command{
	Name:  "token",
	Usage: "Print or rotate claim token associated with your account (authorized devices only)",
	Commands: []*cli.Command{
		dashboardTokenPrintCommand,
		dashboardTokenRotateCommand,
	},
	Category: CategoryApi,
}

var dashboardTokenPrintCommand = &cli.Command{
	Name:      "print",
	Usage:     "Print your claim token",
	ArgsUsage: "",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		resp, err := requests.FetchUserInfo()
		if err != nil {
			printError(err.Error())
			return nil
		}
		printInfo(resp.Payload.Settings.ClaimToken)
		return nil
	},
}

var dashboardTokenRotateCommand = &cli.Command{
	Name:      "rotate",
	Usage:     "Rotate your claim token (you can do it once a minute)",
	ArgsUsage: "",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		resp, err := requests.RotateClaimToken()
		if err != nil {
			printError(err.Error())
			return nil
		}
		printInfo(resp.Payload.Settings.ClaimToken)
		return nil
	},
}
