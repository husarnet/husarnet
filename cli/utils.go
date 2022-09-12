// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"net/netip"
	"os"
	"os/exec"
	"runtime"
	"sort"
	"strings"
	"syscall"
)

// returns true if first slice has equal len() and exactly the same elements as the other one
func areSlicesEqual(first []string, second []string) bool {
	count := len(first)
	if count != len(second) {
		return false
	}

	// yes, inefficient implementation, but
	// the func is only used in tests so far - so no worries yet
	// it will be good refactoring excercise - always look on the bright side of life
	firstCopy := make([]string, count)
	secondCopy := make([]string, count)
	copy(firstCopy, first)
	copy(secondCopy, second)
	sort.Strings(firstCopy)
	sort.Strings(secondCopy)

	for i := 0; i < count; i++ {
		if firstCopy[i] != secondCopy[i] {
			return false
		}
	}
	return true
}

// loops over a slice and returns only the elements that contain given string
func filterSlice(fragment string, slice []string) []string {
	filtered := []string{}

	for i := 0; i < len(slice); i++ {
		if strings.Contains(slice[i], fragment) {
			filtered = append(filtered, slice[i])
		}
	}

	return filtered
}

// checks whether currently running platform is Windows
func onWindows() bool {
	return runtime.GOOS == "windows"
}

func rerunWithSudo() {
	if !askForConfirmation(runSelfWithSudoQuestion) {
		dieEmpty()
	}

	sudoPath, err := exec.LookPath("sudo")
	if err != nil {
		printError("sudo does not appear to be in PATH. Are you sure it is installed?")
		sudoPath = "/usr/bin/sudo"
	}
	err = syscall.Exec(sudoPath, append([]string{sudoPath}, os.Args...), os.Environ())
	if err != nil {
		dieE(err)
	}
}

func runSubcommand(confirm bool, command string, args ...string) {
	logCommandString := command + " " + strings.Join(args, " ")
	if confirm {
		if !askForConfirmation(fmt.Sprintf("Do you want to run: %s?", logCommandString)) {
			dieEmpty()
		}
	} else {
		printInfo(fmt.Sprintf("Running: %s", logCommandString))
	}

	commandPath, err := exec.LookPath(command)
	if err != nil {
		printError(fmt.Sprintf("%s does not appear to be in PATH. Are you sure it is installed?", command))
		commandPath = fmt.Sprintf("/usr/bin/%s", command)
	}

	cmd := exec.Command(commandPath, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		dieE(err)
	}

	if len(output) > 0 {
		printInfo(string(output))
	}
}

func trimNewlines(input string) string {
	input = strings.TrimSuffix(input, "\r\n")
	input = strings.TrimSuffix(input, "\n")

	return input
}

func ShortenJoinCode(originalCode string) string {
	splited := strings.SplitN(originalCode, "/", 2)

	if len(splited) == 1 {
		return splited[0]
	} else if len(splited) == 2 {
		return splited[1]
	} else {
		die("Unknown join code format")
		return "" // hopefully won't happen… :P
	}
}

func makeCannonicalAddr(input string) string {
	addr, err := netip.ParseAddr(input)
	if err != nil {
		dieE(err)
	}

	return addr.StringExpanded()
}
