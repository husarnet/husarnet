// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"github.com/husarnet/husarnet/cli/v2/requests"
	"github.com/husarnet/husarnet/cli/v2/utils"
	"net/netip"
	"os"
	"os/exec"
	"runtime"
	"sort"
	"strings"
	"syscall"

	"github.com/pterm/pterm"
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

// re run the whole CLI invocation with sudo
// note that this function replaces current process with a sudoed one so it won't return to your program in any casse
func rerunWithSudoOrDie() {
	if onWindows() {
		dieEmpty()
	}

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

// run a given command as a subprocess
// will ask user for confirmation if `confirmation` is `true`
func runSubcommand(confirm bool, command string, args ...string) {
	logCommandString := command + " " + strings.Join(args, " ")
	if confirm {
		if !askForConfirmation(pterm.Sprintf("Do you want to run: %s?", logCommandString)) {
			dieEmpty()
		}
	} else {
		printInfo(pterm.Sprintf("Running: %s", logCommandString))
	}

	commandPath, err := exec.LookPath(command)
	if err != nil {
		printError(pterm.Sprintf("%s does not appear to be in PATH. Are you sure it is installed?", command))
		commandPath = fmt.Sprintf("/usr/bin/%s", command)
	}

	cmd := exec.Command(commandPath, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		// TODO long-term/research: figure out how to mimic "sudo" on Windows,
		// so the user does not need to have elevated command prompt for these commands
		// and also we get rid of this awkward error handling
		// NOTE (from pidpawel) I'm using `gsudo` on Windows but AFAIR it's not a standard tool
		errorStringAtNoAdministratorPrivileges := "status 3"
		errorStringAtServiceAlreadyRunning := "status 1"

		if strings.Contains(err.Error(), errorStringAtNoAdministratorPrivileges) {
			printError("Unable to manage the service - are you Administrator?")
			rerunWithSudoOrDie() // At the moment this will be a no-op on Windows
		}

		if strings.Contains(err.Error(), errorStringAtServiceAlreadyRunning) {
			printError("Can't start the service - service already running!")
		}

		dieE(err)
	}

	if len(output) > 0 {
		printInfo(string(output))
	}
}

// trims newline characters from the end of the string
// if the string is multiline only the last line will have it's newline stripped
func trimNewlines(input string) string {
	input = strings.TrimSuffix(input, "\r\n")
	input = strings.TrimSuffix(input, "\n")

	return input
}

// makes sure that given join code is in a short format
// if it already is in a short format - it's returned with no changes
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

// roughly check whether a given string is join code-ish
func isJoinCode(candidate string) bool {
	if len(candidate) == 0 {
		return false
	}

	splited := strings.SplitN(candidate, "/", 2)

	if len(splited) == 2 {
		candidate = splited[1]
	}

	if len(candidate) != 22 {
		return false
	}

	return true
}

// parses an IP v4/v6 address and returns it as a string in a full format
func makeCanonicalAddr(input string) string {
	addr, err := netip.ParseAddr(input)
	if err != nil {
		dieE(err)
	}

	return addr.StringExpanded()
}

// remove duplicates from a given slice
func removeDuplicates[T string](input []T) []T {
	set := make(map[T]struct{})

	for _, it := range input {
		set[it] = struct{}{}
	}

	keys := make([]T, 0, len(set))
	for k := range set {
		keys = append(keys, k)
	}

	return keys
}

// checks whether a given string contains only [0-9a-f]
// won't check the length for full bytes though
func isHexString(s string) bool {
	for _, c := range s {
		if !(('a' <= c && c <= 'f') || ('0' <= c && c <= '9')) {
			return false
		}
	}

	return true
}

// Simply check if given file exists
func fileExists(filename string) bool {
	_, err := os.Stat(filename)
	return err == nil
}

func determineGroupUuid(identifier string) (string, error) {
	if !utils.LooksLikeUuidv4(identifier) {
		resp, err := requests.FetchGroups()
		if err != nil {
			return "", err
		}

		uuid, err := utils.FindGroupUuidByName(identifier, resp.Payload)
		if err != nil {
			return "", err
		}
		return uuid, nil
	}
	return identifier, nil
}

func determineDeviceUuid(identifier string) (string, error) {
	if utils.LooksLikeIpv6(identifier) {
		resp, err := requests.FetchDevices()
		if err != nil {
			return "", nil
		}

		uuid, err := utils.FindDeviceUuidByIp(identifier, resp.Payload)
		if err != nil {
			return "", err
		}
		return uuid, nil
	} else if !utils.LooksLikeUuidv4(identifier) { // we assume it's a hostname (or hostname alias)
		resp, err := requests.FetchDevices()
		if err != nil {
			return "", err
		}

		uuid, err := utils.FindDeviceUuidByHostname(identifier, resp.Payload)
		if err != nil {
			return "", err
		}
		return uuid, nil
	}
	return identifier, nil // yeah, it's probably uuid
}

func determineDeviceIP(identifier string) (string, error) {
	if utils.LooksLikeIpv6(identifier) {
		return identifier, nil
	}

	resp, err := requests.FetchDevices()
	if err != nil {
		return "", err
	}

	for _, dev := range resp.Payload {
		if dev.Id == identifier {
			return dev.Ip, nil
		}
		if dev.Hostname == identifier {
			return dev.Ip, nil
		}
		for _, alias := range dev.Aliases {
			if alias == identifier {
				return dev.Ip, nil
			}
		}
	}

	return "", fmt.Errorf("Couldn't find device identified with '%s'", identifier)
}
