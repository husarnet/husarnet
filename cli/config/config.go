// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package config

import (
	"fmt"
	"github.com/urfave/cli/v3"
	"os"
	"runtime"
)

var defaultDaemonAPIIp = "127.0.0.1"
var defaultDaemonAPIPort int64 = 16216

var husarnetDaemonAPIIp = ""
var husarnetDaemonAPIPort int64 = 0
var daemonApiSecret string

const (
	DaemonApiPortFlagName    = "daemon_api_port"
	DaemonApiAddressFlagName = "daemon_api_address"
	DaemonApiSecretFlagName  = "daemon_api_secret"
)

func Init(cmd *cli.Command) {
	husarnetDaemonAPIIp = cmd.String(DaemonApiAddressFlagName)
	husarnetDaemonAPIPort = cmd.Int(DaemonApiPortFlagName)
	daemonApiSecret = cmd.String(DaemonApiSecretFlagName)
}

func GetDaemonApiIp() string {
	return husarnetDaemonAPIIp
}

func GetDaemonApiPort() int64 {
	return husarnetDaemonAPIPort
}

func GetDaemonApiAddress() string {
	return husarnetDaemonAPIIp
}

func GetDaemonApiSecret() string {
	return daemonApiSecret
}

func GetDaemonApiSecretPath() string {
	if runtime.GOOS == "windows" {
		sep := string(os.PathSeparator)
		return os.ExpandEnv("${programdata}") + sep + "husarnet" + sep + "daemon_api_token"
	}
	// TODO: this path should be configurable
	return "/var/lib/husarnet/daemon_api_token"
}

func GetDaemonApiUrl() string {
	if husarnetDaemonAPIPort == 0 {
		husarnetDaemonAPIPort = defaultDaemonAPIPort
	}

	if husarnetDaemonAPIIp == "" {
		husarnetDaemonAPIIp = defaultDaemonAPIIp
	}

	return fmt.Sprintf("http://%s:%d", husarnetDaemonAPIIp, husarnetDaemonAPIPort)
}
