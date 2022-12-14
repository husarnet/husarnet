// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"os/exec"
	"strings"
)

func getDaemonBinaryPath() string {
	if onWindows() {
		return "husarnet-daemon"
	}

	return "/usr/bin/husarnet-daemon"
}

func promptDaemonRestart() {
	runSubcommand(true, "sudo", "systemctl", "restart", "husarnet")
}

func getDaemonBinaryVersion() string {
	cmd := exec.Command(getDaemonBinaryPath(), "--version")

	output, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Printf("Error while getting daemon version: %v\n", err)
		return "Unknown"
	}

	return strings.TrimSpace(string(output))
}

func getDaemonGenId() string {
	cmd := exec.Command(getDaemonBinaryPath(), "--genid")

	output, err := cmd.CombinedOutput()
	if err != nil {
		die("Error while generating new id: %v\n", err)
	}

	return string(output)
}
