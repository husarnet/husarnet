// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"context"
	"errors"
	"fmt"
	"io/fs"
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

var restoreDefaultsCommand = &cli.Command{
	Name:  "restore",
	Usage: "Restore defaults.ini file to initial state",
	Action: func(ctx context.Context, cmd *cli.Command) error {
		path := getFullPath("defaults.ini")
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
		path := getFullPath("defaults.ini")
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

var resetCommand = &cli.Command{
	Name:  "reset",
	Usage: "Reset the daemon to initial state. This command will restart the daemon.",
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:  "soft",
			Usage: "only clear caches, peer and license information, don't change the environment defaults and service files. Daemon will restart",
		},
		&cli.BoolFlag{
			Name:  "hard",
			Usage: "clear everything",
		},
	},
	Action: func(ctx context.Context, cmd *cli.Command) error {
		softFlag := cmd.Bool("soft")
		hardFlag := cmd.Bool("hard")
		if softFlag && hardFlag {
			printError("set only one of (--soft, --hard) flags")
			return nil
		}

		if !askForConfirmation("This command will briefly stop Husarnet Daemon and change some of the files in Husarnet data folder. Administrator privileges are required. Are you sure you want to proceed?") {
			printInfo("Aborted.")
			return nil
		}

		err := stopDaemon()
		if err != nil {
			return err
		}

		err = os.WriteFile(getFullPath("cache.json"), []byte("{}"), 0660)
		if err != nil {
			return err
		}

		err = os.WriteFile(getFullPath("config.json"), []byte("{}"), 0660)
		if err != nil {
			return err
		}

		if hardFlag {
			// reset defaults
			// delete older, unused files
			err = os.WriteFile(getFullPath("defaults.ini"), []byte(defaultsIniTemplate), 0660)
			if err != nil {
				return err
			}

			// files used by older versions, not needed anymore
			_ = os.Remove(getFullPath("license.json"))
			_ = os.Remove(getFullPath("notifications.json"))
		}

		err = startDaemon()
		if err != nil {
			return err
		}

		return nil
	},
}

// tries to write the file, if fails with permission error displays rerun with sudo prompt
func writeFileWithSudoAttempt(path, contents string, perm os.FileMode) error {
	err := os.WriteFile(path, []byte(contents), perm)
	if err != nil {
		printError("Error: could not write to %s - %s", path, err)
		if errors.Is(err, fs.ErrPermission) {
			rerunWithSudoOrDie()
		} else {
			return err
		}
	}
	return nil
}

func stopDaemon() error {
	if onWindows() {
		runSubcommand(false, "nssm", "stop", "husarnet")
		return nil
	}

	ensureServiceInstalled()
	err := ServiceObject.Stop()
	return err
}

func startDaemon() error {
	// Temporary solution for Windows, until we get rid of nssm
	if onWindows() {
		runSubcommand(false, "nssm", "start", "husarnet")
		return nil
	}

	ensureServiceInstalled()
	err := ServiceObject.Start()
	return err
}
