// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package output

import (
	"encoding/json"
	"fmt"

	"github.com/husarnet/husarnet/cli/v2/types"
	"github.com/pterm/pterm"
)

func PrintSuccess(format string, args ...any) {
	pterm.Success.Printfln(format, args...)
}

func PrintWarning(format string, args ...any) {
	pterm.Warning.Printfln(format, args...)
}

func PrintError(format string, args ...any) {
	pterm.Error.Printfln(format, args...)
}

func PrintGenericApiResponse(resp *types.ApiResponse[any], textIfSuccess, textIfError string) {
	if resp.Type == "success" {
		PrintSuccess(textIfSuccess)
		if len(resp.Warnings) > 0 {
			for _, warning := range resp.Warnings {
				PrintWarning(warning)
			}
		}
	} else {
		PrintError(textIfError)
		if len(resp.Errors) > 0 {
			for _, e := range resp.Errors {
				PrintError(e)
			}
		}
	}

}

func PrintJsonOrError(data any, pretty bool) {
	var jsonBytes []byte
	// we will use this function only with data previously successfully unmarshalled
	// so the error can be safely ignored
	if pretty {
		jsonBytes, _ = json.MarshalIndent(data, "", "  ")
	} else {
		jsonBytes, _ = json.Marshal(data)
	}
	fmt.Println(string(jsonBytes))
}
