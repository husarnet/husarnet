// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"runtime"

	"github.com/kardianos/service"
	"github.com/urfave/cli/v2"
)

const systemdScriptTemplate = `[Unit]
Description={{.Description}}
After=network-online.target
Wants=network-online.target

[Service]
Type=notify
ExecStart={{.Path|cmdEscape}}{{range .Arguments}} {{.|cmd}}{{end}}
Restart=always
RestartSec=3
NotifyAccess=all

[Install]
WantedBy=multi-user.target
`

// Note: it's not like we can choose it - path prefix is hardcoded in the service library.
const systemdUnitFilePath = "/etc/systemd/system/husarnet.service"
const launchctlServiceFilePath = "/Library/LaunchDaemons/husarnet.plist"

// Although service library provides "Executable" option, so we can point to daemon binary instead of CLI,
// we still have to align to interface as it would be *the CLI* that is managed as a system service.
// Hence, this stubs for Start() and Stop() methods.
type program struct{}

func (p *program) Start(s service.Service) error {
	return nil
}

func (p *program) Stop(s service.Service) error {
	return nil
}

var ServiceObject = makeService()

func makeService() service.Service {
	serviceConfig := &service.Config{
		Name:        "husarnet",
		DisplayName: "Husarnet",
		Description: "Husarnet",
	}

	serviceConfig.Executable = getDaemonBinaryPath()

	switch runtime.GOOS {
	case "linux":
		serviceConfig.Option = service.KeyValue{
			"SystemdScript": systemdScriptTemplate,
		}
	case "darwin":
		serviceConfig.Option = service.KeyValue{
			"LogDirectory": "/tmp/",
		}
	case "windows":
		// serviceConfig.Executable = "husarnet-daemon.exe" // must be in %PATH%, which is not obvious I guess
		serviceConfig.UserName = "LocalSystem"
	default:
		printWarning("Unsupported OS for service installation. User should manually install the service.")
	}

	prg := &program{}

	s, err := service.New(prg, serviceConfig)
	if err != nil {
		printError("Could not create Service object for OS service manager")
	}
	return s
}

// This function does the heavy lifting when it comes to figuring out the situation in the system
// Might rename and/or redesign it later
func isAlreadyInstalled(silent bool) bool {
	switch runtime.GOOS {
	case "linux":
		if fileExists(systemdUnitFilePath) {
			if !silent {
				printInfo("Found service file in: " + systemdUnitFilePath)
			}
			return true
		}
		return false
	case "darwin":
		if fileExists(launchctlServiceFilePath) {
			if !silent {
				printInfo("Found service file in: " + launchctlServiceFilePath)
			}
			return true
		}
	case "windows":
		// TODO: implement on Windows.
		printInfo("Not implemented yet.")
	default:
		printWarning("Unsupported OS for service installation. User should manually install the service.")
	}
	return false
	// TODO: maybe check also if daemon is running, to inform the user properly.
}

var daemonServiceInstallCommand = &cli.Command{
	Name:      "service-install",
	Usage:     "install service definition in OS's service manager (e.g. systemctl on Linux)",
	ArgsUsage: " ", // No arguments needed
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:  "silent",
			Usage: "Don't print the usual SUCCESS/FAIL statements.",
		},
		&cli.BoolFlag{
			Name:  "force",
			Usage: "Don't check if the unit/service file exists already before attempting to write",
		},
	},
	Action: func(ctx *cli.Context) error {
		silentFlag := ctx.Bool("silent")
		forceFlag := ctx.Bool("force")

		if !forceFlag && isAlreadyInstalled(silentFlag) {
			if !silentFlag {
				printInfo("Service already exists. Aborting the installation.")
			}
			return nil
		}

		err := ServiceObject.Install()
		if err != nil {
			if !silentFlag {
				printError("An error occurred while installing service.")
				dieE(err)
			}
			dieEmpty()
		}

		if !silentFlag {
			printSuccess("Service installed successfully!")
		}

		// we are starting the service right away - why wait?
		err = ServiceObject.Start()
		if err != nil {
			if !silentFlag {
				printWarning("Service installed, but could not start. Check the status of the service.")
			}
			return err
		}
		return nil
	},
}

var daemonServiceUninstallCommand = &cli.Command{
	Name:      "service-uninstall",
	Usage:     "remove service definition from OS's service manager (e.g. systemctl on Linux)",
	ArgsUsage: " ", // No arguments needed
	Flags: []cli.Flag{
		&cli.BoolFlag{
			Name:  "silent",
			Usage: "Don't print the usual SUCCESS/FAIL statements.",
		},
		&cli.BoolFlag{
			Name:  "force",
			Usage: "Don't check if the unit/service file exists already before attempting to write",
		},
	},
	Action: func(ctx *cli.Context) error {
		silentFlag := ctx.Bool("silent")
		forceFlag := ctx.Bool("force")

		if !forceFlag && runtime.GOOS == "linux" && !fileExists(systemdUnitFilePath) {
			if !silentFlag {
				printWarning("Service does not seem to be installed. Not attempting uninstallation.")
			}
			return nil
		}

		err := ServiceObject.Stop()
		if err != nil {
			if !silentFlag {
				printError("An error occurred while stopping service.")
			}
		}
		err = ServiceObject.Uninstall()
		if err != nil {
			if !silentFlag {
				printError("An error occurred while removing service.")
				dieE(err)
			}
			dieEmpty()
		}

		if !silentFlag {
			printSuccess("Service removed.")
		}

		return nil
	},
}
