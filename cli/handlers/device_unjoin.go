// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package handlers

import (
	"fmt"
	"hdm/generated"

	"github.com/Khan/genqlient/graphql"
)

type UnjoinDeviceHandler struct{}

func (h UnjoinDeviceHandler) PerformRequest(client graphql.Client, args ...string) (interface{}, error) {
	return generated.KickDevice(client, args[0], args[1])
}

func (h UnjoinDeviceHandler) PrintResults(data interface{}) {
	fmt.Println("Device was successfully removed from the group.")
}
