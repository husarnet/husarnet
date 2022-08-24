// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"os"
	"syscall"

	"github.com/pterm/pterm"
	"github.com/urfave/cli/v2"
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

func ignoreExtraParameters(ctx *cli.Context) {
	if ctx.Args().Len() > 0 {
		cli.ShowSubcommandHelp(ctx)
		os.Exit(1)
	}
}

func printSuccess(text string) {
	pterm.Success.Println(text)
}

func printError(text string) {
	pterm.Error.Println(text)
}

func printErrorE(err error) {
	pterm.Error.Println(err.Error())
}

func printInfo(text string) {
	pterm.Info.WithPrefix(pterm.Prefix{
		Style: &pterm.ThemeDefault.InfoPrefixStyle,
		Text:  " INFO  ",
	}).Println(text)
}

func printParagraph(text string) {
	pterm.DefaultParagraph.Println(text)
}

// exit the program with the given message and status code 1.
func die(message string) {
	printError(message)
	dieEmpty()
}

// die, printing exception
func dieE(err error) {
	die(fmt.Sprintf("%v\n", err))
}

func dieEmpty() {
	os.Exit(1)
}

// prompts user for username and password and returns them. Password is not visible while typing
func getUserCredentialsFromStandardInput() (string, string) {
	printParagraph(credentialsPrompt)
	username, err := pterm.DefaultInteractiveTextInput.WithMultiLine(false).Show("Username")
	if err != nil {
		dieE(err)
	}
	pterm.Println()

	pterm.ThemeDefault.PrimaryStyle.Print("Password: ")
	bytePassword, err := term.ReadPassword(int(syscall.Stdin))
	if err != nil {
		dieE(err)
	}
	password := string(bytePassword)
	pterm.Println()

	return trimNewlines(username), trimNewlines(password)
}

// prompts user for confirmation and exits the whole program if user does not confirm
func askForConfirmation(question string) bool {
	result, err := pterm.DefaultInteractiveConfirm.Show(question)
	if err != nil {
		dieE(err)
	}

	return result
}
