// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"os"
	"time"

	"github.com/mattn/go-runewidth"
	"github.com/pterm/pterm"
	"github.com/urfave/cli/v3"
)

var redFormatter = pterm.Red
var yellowFormatter = pterm.Yellow
var greenFormatter = pterm.Green
var neutralFormatter = pterm.Normal

var redDot = redFormatter("●")
var yellowDot = yellowFormatter("●")
var greenDot = greenFormatter("●")
var neutralDot = neutralFormatter("●")

var redStyle = pterm.NewStyle(pterm.FgRed)
var yellowStyle = pterm.NewStyle(pterm.FgLightYellow)
var greenStyle = pterm.NewStyle(pterm.FgLightGreen)
var neutralStyle = pterm.NewStyle()

// Given the coloured dot, return a matching formatter
func extractFormatter(dot string) func(...interface{}) string {
	var formatter func(...interface{}) string

	if dot == redDot {
		formatter = redFormatter
	} else if dot == yellowDot {
		formatter = yellowFormatter
	} else if dot == greenDot {
		formatter = greenFormatter
	} else {
		formatter = neutralFormatter
	}

	return formatter
}

// Change the required default for pterm and other libraries
func initTheme() {
	if verboseLogs {
		pterm.EnableDebugMessages()
	}

	theme := &pterm.ThemeDefault
	theme.PrimaryStyle = *pterm.NewStyle(pterm.FgLightRed)
	theme.SecondaryStyle = *pterm.NewStyle(pterm.FgRed)

	theme.DescriptionMessageStyle = theme.DefaultText
	theme.DescriptionPrefixStyle = *neutralStyle
	pterm.Description.Prefix.Text = "DESCRIPTION:"

	theme.DebugMessageStyle = theme.DefaultText
	theme.DebugPrefixStyle = *neutralStyle
	pterm.Debug.Prefix.Text = "DEBUG:"

	theme.SuccessMessageStyle = theme.DefaultText
	theme.SuccessPrefixStyle = *greenStyle
	pterm.Success.Prefix.Text = "SUCCESS:"

	theme.InfoMessageStyle = theme.DefaultText
	theme.InfoPrefixStyle = *neutralStyle
	pterm.Info.Prefix.Text = "INFO:"

	theme.WarningMessageStyle = theme.DefaultText
	theme.WarningPrefixStyle = *yellowStyle
	pterm.Warning.Prefix.Text = "WARNING:"

	theme.ErrorMessageStyle = theme.DefaultText
	theme.ErrorPrefixStyle = *redStyle
	pterm.Error.Prefix.Text = "ERROR:"

	theme.FatalMessageStyle = theme.DefaultText
	theme.FatalPrefixStyle = *redStyle
	pterm.Fatal.Prefix.Text = "FATAL:"

	theme.TableSeparatorStyle = *pterm.NewStyle(pterm.FgDarkGray)
	theme.TableHeaderStyle = theme.PrimaryStyle

	spinner := &pterm.DefaultSpinner
	spinner.TimerRoundingFactor = time.Millisecond
	spinner.RemoveWhenDone = false

	confirmStyle := &pterm.DefaultInteractiveConfirm
	confirmStyle.TextStyle = &theme.DefaultText
	confirmStyle.SuffixStyle = &theme.PrimaryStyle
	confirmStyle.ConfirmStyle = greenStyle
	confirmStyle.RejectStyle = redStyle
}

// Throw an error, show help and die if a different number of arguments than specified is provided
func requiredArgumentsNumber(cmd *cli.Command, number int) {
	if cmd.Args().Len() != number {
		printError("Wrong number of arguments provided. Provided " + pterm.Bold.Sprintf("%d", cmd.Args().Len()) + ", required: " + pterm.Bold.Sprintf("%d", number))
		cli.ShowSubcommandHelp(cmd)
		dieEmpty()
	}
}

// Throw an error, show help and die if number of arguments provided is not within a given range
func requiredArgumentsRange(cmd *cli.Command, lower, upper int) {
	if cmd.Args().Len() < lower || cmd.Args().Len() > upper {
		printError("Wrong number of arguments provided. Provided " + pterm.Bold.Sprintf("%d", cmd.Args().Len()) + ", required between " + pterm.Bold.Sprintf("%d", lower) + " and " + pterm.Bold.Sprintf("%d", upper))
		cli.ShowSubcommandHelp(cmd)
		dieEmpty()
	}
}

// Throw an error, show help and die if any arguments are provided
func ignoreExtraArguments(cmd *cli.Command) {
	if cmd.Args().Len() > 0 {
		printError("Too many arguments provided!")
		cli.ShowSubcommandHelp(cmd)
		dieEmpty()
	}
}

// Throw an error, show help and die if less than `lower` arguments are provided
func minimumArguments(cmd *cli.Command, lower int) {
	if cmd.Args().Len() < lower {
		printError("Not enough arguments provided!")
		cli.ShowSubcommandHelp(cmd)
		dieEmpty()
	}
}

// Print a success message
func printSuccess(format string, args ...interface{}) {
	pterm.Success.Printfln(format, args...)
}

// Print an error message (text version)
func printError(format string, args ...interface{}) {
	pterm.Error.Printfln(format, args...)
}

// Print an warning-level message
func printWarning(format string, args ...interface{}) {
	pterm.Warning.Printfln(format, args...)
}

// Print an info-level message
func printInfo(format string, args ...interface{}) {
	pterm.Info.Printfln(format, args...)
}

// Print an debug-level message
func printDebug(format string, args ...interface{}) {
	if verboseLogs {
		pterm.Debug.Printfln(format, args...)
	}
}

// Print a paragraph
func printParagraph(format string, args ...interface{}) {
	pterm.Description.Printfln(format, args...)
}

// Exit the program with the given message and status code 1
func die(format string, args ...interface{}) {
	printError(format, args...)
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

// Prompts user for confirmation and exits the whole program if user does not confirm
func askForConfirmation(question string) bool {
	if nonInteractive {
		pterm.Println("Running in non-interactive mode, selecting 'NO', as an answer to: " + question)
		dieEmpty()
	}

	pterm.Println()
	// pterm.Printf(" %s   QUESTION   %s  ", neutralDot, neutralDot)
	result, err := pterm.DefaultInteractiveConfirm.Show(question)
	if err != nil {
		dieE(err)
	}
	pterm.Println()

	return result
}

// Gives you an options to interactively choose from
func interactiveChooseFrom(title string, options []string) string {
	widget := pterm.DefaultInteractiveSelect.WithDefaultText(title)

	pterm.Println()
	selected, err := widget.WithOptions(options).Show()
	if err != nil {
		dieE(err)
	}

	return selected
}

// Asks user for a text input
func interactiveTextInput(title string) string {
	pterm.Println()
	response, err := pterm.DefaultInteractiveTextInput.WithDefaultText(title).Show()
	if err != nil {
		dieE(err)
	}
	pterm.Println()
	pterm.Println()

	return response
}

// Return a string length ignoring all utf-8 quirks and terminal formatting
func runeLength(s string) int {
	return runewidth.StringWidth(pterm.RemoveColorFromString(s))
}

func getSpinner(title string, verbose bool) *pterm.SpinnerPrinter {
	spinner, err := pterm.DefaultSpinner.Start(title)
	if err != nil {
		dieE(err)
	}

	if !verboseLogs && !verbose {
		spinner.RemoveWhenDone = true
	}

	return spinner
}
