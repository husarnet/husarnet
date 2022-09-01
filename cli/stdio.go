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

// Throw an error, show help and die if a different number of arguments than specified is provided
func requiredArgumentsNumber(ctx *cli.Context, number int) {
	if ctx.Args().Len() != number {
		printError("Wrong number of arguments provided. Provided " + pterm.Bold.Sprintf("%d", ctx.Args().Len()) + ", required: " + pterm.Bold.Sprintf("%d", number))
		cli.ShowSubcommandHelp(ctx)
		dieEmpty()
	}
}

// Throw an error, show help and die if number of arguments provided is not within a given range
func requiredArgumentsRange(ctx *cli.Context, lower, upper int) {
	if ctx.Args().Len() < lower || ctx.Args().Len() > upper {
		printError("Wrong number of arguments provided. Provided " + pterm.Bold.Sprintf("%d", ctx.Args().Len()) + ", required between " + pterm.Bold.Sprintf("%d", lower) + " and " + pterm.Bold.Sprintf("%d", upper))
		cli.ShowSubcommandHelp(ctx)
		dieEmpty()
	}
}

// Throw an error, show help and die if any arguments are provided
func ignoreExtraArguments(ctx *cli.Context) {
	if ctx.Args().Len() > 0 {
		printError("Too many arguments provided!")
		cli.ShowSubcommandHelp(ctx)
		dieEmpty()
	}
}

// If verbose logs are enabled, pass the arguments to fmt.Println(). Otherwise do nothing
func logV(args ...interface{}) {
	if verboseLogs {
		fmt.Print("[verbose log] ")
		fmt.Println(args...)
	}
}

// Print not implemented yet warning
func notImplementedYet() {
	fmt.Println("Not implemented yet")
}

// Print a success message
func printSuccess(text string) {
	pterm.Success.Println(text)
}

// Print an error message (text version)
func printError(text string) {
	pterm.Error.Println(text)
}

// Print an error message (error type version)
func printErrorE(err error) {
	pterm.Error.Println(err.Error())
}

// Print an info-level message
func printInfo(text string) {
	pterm.Info.WithPrefix(pterm.Prefix{
		Style: &pterm.ThemeDefault.InfoPrefixStyle,
		Text:  " INFO  ",
	}).Println(text)
}

// Print a paragraph
func printParagraph(text string) {
	pterm.DefaultParagraph.Println(text)
}

// Exit the program with the given message and status code 1
func die(message string) {
	printError(message)
	dieEmpty()
}

// Exit the program, outputing a given error first
func dieE(err error) {
	die(fmt.Sprintf("%v\n", err))
}

// Exit the program
func dieEmpty() {
	os.Exit(1)
}

// Prompts user for username and password and returns them. Password is not visible while typing
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

// Prompts user for confirmation and exits the whole program if user does not confirm
func askForConfirmation(question string) bool {
	result, err := pterm.DefaultInteractiveConfirm.Show(question)
	if err != nil {
		dieE(err)
	}

	return result
}
