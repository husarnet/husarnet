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

func PrintJsonOrError(data any) {
	jsonBytes, err := json.Marshal(data)
	if err != nil {
		// we will use this function only with data previously successfully unmarshalled
		// so this should never happen in practice; checking doesn't hurt though
		fmt.Println(err)
	}
	fmt.Println(string(jsonBytes))
}
