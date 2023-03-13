// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"github.com/kardianos/service"
	"github.com/urfave/cli/v2"
	"os"
	"runtime"
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

	ourPath, err := os.Executable()
	if err != nil {
		printWarning("Error during service creation - cannot determine executable path")
		// let's not die here, and just hope for the best :)
		ourPath = "husarnet"
	}

	// Figure out daemon executable path, which should be alongside the cli, with "-daemon" suffix.
	serviceConfig.Executable = replaceLastOccurrence("husarnet", "husarnet-daemon", ourPath)

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
func isAlreadyInstalled() bool {
	switch runtime.GOOS {
	case "linux":
		if fileExists(systemdUnitFilePath) {
			printInfo("Found service file in: " + systemdUnitFilePath)
			return true
		}

		oldLocation := "/lib/systemd/system/husarnet.service"
		if fileExists(oldLocation) {
			// this branch is probably unnecessary, as old post-remove script during update will remove the unit file.
			// I am leaving this just to figure out that's exactly the case, because apt/dpkg sometimes have weird ideas.
			printInfo("Found previous installation of husarnet service, moving...")
			// file exists in old location, move it to new location. sudo is required
			runSubcommand(false, "sudo", "systemctl", "stop", "husarnet")
			runSubcommand(false, "sudo", "systemctl", "disable", "husarnet")
			runSubcommand(false, "sudo", "systemctl", "daemon-reload")
			// user will enter his credentials, so from this point sudo is guaranteed? (this does not feel right but I gotta test it)
			err := os.Rename(oldLocation, systemdUnitFilePath)
			if err != nil {
				printError("Error during service installation. Try again. Error: %s", err)
			}
			return true
		}
		return false
	case "darwin":
		// TODO: remove duplication
		serviceFilePath := "/Library/LaunchDaemons/husarnet.plist"
		_, err := os.Stat(serviceFilePath)
		if err == nil {
			printInfo("Found service file in: " + serviceFilePath)
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

var serviceInstallCommand = &cli.Command{
	Name:      "service-install",
	Usage:     "install service definition in OS's service manager (e.g. systemctl on Linux)",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx *cli.Context) error {
		// I think we must first figure out if we are admin/root.
		// "install" or "uninstall" should not be run as not admin

		if isAlreadyInstalled() {
			printInfo("Service already exists. Aborting the installation.")
			return nil
		}

		err := ServiceObject.Install()
		if err != nil {
			printError("An error occurred while installing service.")
			dieE(err)
		}

		// TODO: We should probably consider having --quiet switch, so we don't spam during apt upgrades
		printSuccess("Service installed successfully!")

		// we are starting the service right away - why wait?
		err = ServiceObject.Start()
		if err != nil {
			return err
		}

		return nil
	},
}

var serviceUninstallCommand = &cli.Command{
	Name:      "service-uninstall",
	Usage:     "remove service definition from OS's service manager (e.g. systemctl on Linux)",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx *cli.Context) error {
		// Attempt uninstallation only when service file is present...
		// specific for systemd...
		if runtime.GOOS == "linux" && !fileExists(systemdUnitFilePath) {
			printWarning("Service does not seem to be installed. Not attempting uninstallation.")
			return nil
		}

		err := ServiceObject.Uninstall()
		if err != nil {
			printError("An error occurred while removing service.")
			dieE(err)
		}

		printSuccess("Service removed.")

		return nil
	},
}
