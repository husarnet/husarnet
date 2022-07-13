// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"bufio"
	"fmt"
	"os"
	"strings"
	"syscall"

	"golang.org/x/term"
)

// if verbose logs are enabled, pass the arguments to fmt.Println(). Otherwise do nothing
func logV(args ...interface{}) {
	if verboseLogs {
		fmt.Print("[verbose log] ")
		fmt.Println(args...)
	}
}

// print not implemented yet warning
func notImplementedYet() {
	fmt.Println("Not implemented yet")
}

// exit the program with the given message and status code 1.
func die(message string) {
	fmt.Println(message)
	os.Exit(1)
}

// die, printing exception
func dieE(err error) {
	die(fmt.Sprintln("%v", err))
}

// prompts user for username and password and returns them. Password is not visible while typing
func getUserCredentialsFromStandardInput() (string, string) {
	fmt.Println(credentialsPrompt)
	reader := bufio.NewReader(os.Stdin)
	fmt.Print("Username: ")
	username, _ := reader.ReadString('\n')
	fmt.Print("Password: ")
	bytePassword, _ := term.ReadPassword(int(syscall.Stdin))
	username = strings.TrimSuffix(username, "\r\n")
	username = strings.TrimSuffix(username, "\n")
	password := strings.TrimSuffix(string(bytePassword), "\r\n")
	password = strings.TrimSuffix(string(bytePassword), "\n")
	fmt.Println()
	return username, password
}

// prompts user for confirmation and exits the whole program if user does not confirm
func askForConfirmation(question string) {
	fmt.Print(question + " [y/N]: ")
	reader := bufio.NewReader(os.Stdin)
	decision, _ := reader.ReadString('\n')
	decision = strings.ToLower(strings.TrimSpace(decision))

	if decision != "y" && decision != "yes" {
		die("Operation aborted.")
	}
}
