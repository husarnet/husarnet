// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package dashboard_handlers

import (
	"fmt"
	"hdm/generated"

	"github.com/Khan/genqlient/graphql"
)

type RenameGroupHandler struct{}

func (h RenameGroupHandler) PerformRequest(client graphql.Client, args ...string) (interface{}, error) {
	return generated.RenameGroup(client, args[0], args[1])
}

func (h RenameGroupHandler) PrintResults(data interface{}) {
	fmt.Println("Group renamed successfully.")
}
