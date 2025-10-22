// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"fmt"
	"os"
	"os/exec"

	"github.com/husarnet/husarnet/cli/v2/config"
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

var defaultsIniTemplate = `; consult the documentation before editing. See https://husarnet.com/docs/defaults-ini

; Husarnet INI file, which is read both by the Daemon and the CLI on startup
; this is convenient way of providing a default value for an environment variable used by Husarnet
; the values that are provided here are the defaults, which means env vars set directly on the running process
; (for example via /etc/default/husarnet on Linux) will override them.

[common]
; instance_fqdn = prod.husarnet.com
; log_verbosity = info
; enable_hooks = false

[daemon]
; daemon_interface = hnet0

[cli]
; daemon_api_secret = some-secret`

var restoreDefaultsCommand = &cli.Command{
	Name:  "restore",
	Usage: "Restore defaults.ini file to initial state",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		path := config.GetDefaultsIniPath()
		if !askForConfirmation(fmt.Sprintf("This operation will overwrite the contents of %s. Are you sure?", path)) {
			dieEmpty()
		}

		file, err := os.OpenFile(path, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0644)
		if err != nil {
			printError("can't open file %s for writing - are you root/Administrator?", path)
			return err
		}

		_, err = file.WriteString(defaultsIniTemplate)
		if err != nil {
			printError("an error occurred while writing file %s", path)
			return err
		}

		printSuccess("file %s has been reset", path)
		return nil
	},
}

var editDefaultsCommand = &cli.Command{
	Name:  "edit",
	Usage: "Open default editor to examine (or edit) default values for environment variables",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		path := config.GetDefaultsIniPath()
		file, err := os.OpenFile(path, os.O_RDWR, 0644)
		if err != nil {
			printError("can't open file %s - you need administrative privileges to open this file (or the file does not exist, in this case run `husarnet defaults restore` to restore the file", path)
			return err
		}

		err = file.Close()
		if err != nil {
			printError("an error occurred while testing the access to file %s", path)
			return err
		}

		editorExecutable := config.GetDefaultEditorPath()
		editorCommand := exec.Command(editorExecutable, path)
		editorCommand.Stdin = os.Stdin
		editorCommand.Stdout = os.Stdout
		editorCommand.Stderr = os.Stderr
		err = editorCommand.Start()
		if err != nil {
			printError("can't start editor - %s failed", editorExecutable+" "+path)
		}
		err = editorCommand.Wait()
		if err != nil {
			printError("error while waiting for subcommand (%s) to finish", editorExecutable+" "+path)
		} else {
			printSuccess("editing successful. Restart Husarnet Daemon to apply the changes (execute `husarnet restart`)")
		}

		return nil
	},
}
