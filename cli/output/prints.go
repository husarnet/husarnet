package output

import (
	"encoding/json"
	"fmt"
)

func PrintJsonOrError(data any) {
	jsonBytes, err := json.Marshal(data)
	if err != nil {
		// we will use this function only with data previously successfully unmarshalled
		// so this should never happen in practice; checking doesn't hurt though
		fmt.Println(err)
	}
	fmt.Println(string(jsonBytes))
}
