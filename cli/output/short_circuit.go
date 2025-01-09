package output

import (
	"fmt"
	"github.com/pterm/pterm"
	"os"
)

// Print an error message (text version)
func Error(format string, args ...interface{}) {
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

// Exit the program with the given message and status code 1
func Die(format string, args ...interface{}) {
	Error(format, args...)
	dieEmpty()
}

// DieWithError will exit the program, outputing a given error first
func DieWithError(err error) {
	Die(fmt.Sprintf("%v\n", err))
}

// Exit the program
func dieEmpty() {
	os.Exit(1)
}
