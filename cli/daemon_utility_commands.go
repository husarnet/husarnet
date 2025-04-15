// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"

	"github.com/pterm/pterm"
	"github.com/urfave/cli/v3"
)

var daemonIpCommand = &cli.Command{
	Name:      "ip",
	Usage:     "Print Husarnet ip address of the daemon (no arguments) or a known peer (with single argument of peer hostname)",
	ArgsUsage: "[peer hostname]",
	Category:  CategoryUtils,
	Action: func(ctx context.Context, cmd *cli.Command) error {
		requiredArgumentsRange(cmd, 0, 1)

		if cmd.Args().Len() == 0 {
			status := getDaemonStatus()
			pterm.Println(status.LiveData.LocalIP.StringExpanded())
		} else {
			arg := cmd.Args().First()
			// TODO we'll be using HostTable field, but it has to consider aliases too, which is not implemented
			status := getDaemonStatus()
			found := false
			for k, v := range status.HostTable {
				if k == arg {
					pterm.Println(v)
					found = true
				}
			}
			if !found {
				printError("no peer is known as %s", arg)
			}
		}

		return nil
	},
}

var versionCommand = &cli.Command{
	Name:  "version",
	Usage: "Print the version of the CLI and also of the daemon, if available",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		printVersion(getDaemonRunningVersion())
		return nil
	},
}
