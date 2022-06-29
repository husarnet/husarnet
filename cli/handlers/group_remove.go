// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package handlers

import (
	"fmt"
	"hdm/generated"

	"github.com/Khan/genqlient/graphql"
)

type RemoveGroupHandler struct{}

func (h RemoveGroupHandler) PerformRequest(client graphql.Client, args ...string) (interface{}, error) {
	return generated.RemoveGroup(client, args[0])
}

func (h RemoveGroupHandler) PrintResults(data interface{}) {
	fmt.Println("Group removed successfully.")
}
