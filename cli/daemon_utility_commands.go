// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"github.com/pterm/pterm"
	"github.com/urfave/cli/v2"
)

var daemonIpCommand = &cli.Command{
	Name:      "ip",
	Usage:     "print Husarnet ip address of the daemon (no arguments) or known peer (with single argument of peer hostname)",
	ArgsUsage: "[peer hostname]",
	Action: func(ctx *cli.Context) error {
		requiredArgumentsRange(ctx, 0, 1)

		if ctx.Args().Len() == 0 {
			status := getDaemonStatus()
			pterm.Println(status.LocalIP.StringExpanded())
		} else {
			arg := ctx.Args().First()
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
