// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"os"
	"os/exec"
	"strings"
)

func getFallbackDaemonBinaryPath() string {
	if onWindows() {
		return "husarnet-daemon"
	}

	return "/usr/bin/husarnet-daemon"
}

func getDaemonBinaryPath() string {
	ourPath, err := os.Executable()
	if err != nil {
		printWarning("System problem: could not determine executable path")
		// let's not die here, and just hope for the best :)
		return getFallbackDaemonBinaryPath()
	}

	potentialDaemonBinaryPath := replaceLastOccurrence("husarnet", "husarnet-daemon", ourPath)

	if fileExists(potentialDaemonBinaryPath) {
		return potentialDaemonBinaryPath
	}

	return getFallbackDaemonBinaryPath()
}

func restartDaemonWithConfirmationPrompt() {
	if !askForConfirmation("Do you want to restart Husarnet Daemon now?") {
		dieEmpty()
	}

	err := ServiceObject.Restart()
	if err != nil {
		printWarning("Wasn't able to restart Husarnet Daemon. Try restarting the service manually.")
	}

	waitDaemon()
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

func getDaemonBinaryGenIdOutput() string {
	cmd := exec.Command(getDaemonBinaryPath(), "--genid")

	output, err := cmd.CombinedOutput()
	if err != nil {
		die("Error while generating new id: %v\n", err)
	}

	return string(output)
}
