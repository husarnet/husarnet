package main

import (
	"github.com/kardianos/service"
	"github.com/urfave/cli/v2"
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
		Name:        "husarnet-nightly", // suffix for development so we don't conflict - TODO change when feature complete
		DisplayName: "Husarnet",
		Description: "Husarnet",
	}

	switch runtime.GOOS {
	case "linux":
		serviceConfig.Option = service.KeyValue{
			"SystemdScript": systemdScriptTemplate,
		}
		serviceConfig.Executable = "/usr/bin/husarnet-daemon"
	case "darwin":
		serviceConfig.Option = service.KeyValue{
			"LogDirectory": "/tmp/",
		}
	case "windows":
		serviceConfig.Executable = "husarnet-daemon.exe" // must be in %PATH%, which is not obvious I guess
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

func isAlreadyInstalled() bool {
	// might come in handy later, TODO implement
	// /lib/systemd/system/husarnet.service
	// /etc/systemd/system/husarnet.service
	return false
	// TODO: maybe check also if daemon is running, to inform the user properly.
}

var installCommand = &cli.Command{
	Name:      "install",
	Usage:     "install service definition in current platform's service manager (e.g. systemctl on Linux)",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx *cli.Context) error {
		// TODO: find out if husarnet is already in systemctl.
		// if yes we probably should not touch it.

		err := ServiceObject.Install()
		if err != nil {
			printError("An error occurred while installing service.")
			dieE(err)
		}

		printSuccess("Service installed successfully!")

		// we are starting the service right away - why wait?
		err = ServiceObject.Start()
		if err != nil {
			return err
		}

		return nil
	},
}

var uninstallCommand = &cli.Command{
	Name:      "uninstall",
	Usage:     "remove service definition from current platform's service manager (e.g. systemctl on Linux)",
	ArgsUsage: " ", // No arguments needed
	Action: func(ctx *cli.Context) error {
		err := ServiceObject.Uninstall()
		if err != nil {
			printError("An error occurred while removing service.")
			dieE(err)
		}
		printSuccess("Service removed.")

		return nil
	},
}
