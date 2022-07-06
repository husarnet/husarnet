// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package dashboard_handlers

import (
	"fmt"
	"hdm/generated"

	"github.com/Khan/genqlient/graphql"
)

type CreateGroupHandler struct{}

func (h CreateGroupHandler) PerformRequest(client graphql.Client, args ...string) (interface{}, error) {
	return generated.CreateGroup(client, args[0])
}

func (h CreateGroupHandler) PrintResults(data interface{}) {
	resp := data.(*generated.CreateGroupResponse)

	fmt.Println("Group created. New group information:")
	fmt.Println("{")
	fmt.Println("\tId:", resp.CreateGroup.Group.Id)
	fmt.Println("\tName:", resp.CreateGroup.Group.Name)
	fmt.Println("}")
}
