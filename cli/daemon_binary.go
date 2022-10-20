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

func getDaemonBinaryVersion() string {
	cmd := exec.Command(getDaemonBinaryPath(), "--version")

	output, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Printf("Error while getting daemon version: %v\n", err)
		return "Unknown"
	}

	return strings.TrimSpace(string(output))
}
