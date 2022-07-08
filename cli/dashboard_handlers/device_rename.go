// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package dashboard_handlers

import (
	"fmt"
	"hdm/generated"

	"github.com/Khan/genqlient/graphql"
)

type RenameDeviceHandler struct{}

func (h RenameDeviceHandler) PerformRequest(client graphql.Client, args ...string) (interface{}, error) {
	return generated.RenameDevice(client, args[0], args[1])
}

func (h RenameDeviceHandler) PrintResults(data interface{}) {
	fmt.Println("Device renamed successfully.")
}
