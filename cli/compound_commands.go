// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"net/url"

	"github.com/urfave/cli/v2"
)

var joinCommand = &cli.Command{
	Name:      "join",
	Usage:     "Connect to Husarnet group with given join code and with specified hostname",
	ArgsUsage: "[join code] [device name]",
	Action: func(ctx *cli.Context) error {
		requiredArgumentsRange(ctx, 0, 2)

		var joinCode, hostname string

		if ctx.Args().Len() >= 1 {
			if isJoinCode(ctx.Args().Get(0)) {
				joinCode = ctx.Args().Get(0)
			} else {
				joinCode = getGroupByName(ctx.Args().Get(0)).JoinCode
			}
		} else {
			joinCode = interactiveGetGroupByName().JoinCode
		}

		if ctx.Args().Len() >= 2 {
			hostname = ctx.Args().Get(1)
		} else {
			hostname = ""
		}

		callDaemonPost[EmptyResult]("/api/join", url.Values{
			"code":     {joinCode},
			"hostname": {hostname},
		})

		printSuccess("Successfully registered a join request")
		waitBaseANY()
		waitWebsetup()
		waitJoined()

		return nil
	},
}
