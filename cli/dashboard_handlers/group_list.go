// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package dashboard_handlers

import (
	"fmt"
	"hdm/generated"

	"github.com/Khan/genqlient/graphql"
	"github.com/rodaine/table"
)

type ListGroupsHandler struct{}

func (h ListGroupsHandler) PerformRequest(client graphql.Client, args ...string) (interface{}, error) {
	return generated.GetGroups(client)
}

func (h ListGroupsHandler) PrintResults(data interface{}) {
	resp := data.(*generated.GetGroupsResponse)
	groupsList := resp.Groups
	fmt.Println("Your groups:")

	tbl := table.New("ID", "Name", "DeviceCount", "JoinCode")

	for i := 0; i < len(groupsList); i++ {
		tbl.AddRow(groupsList[i].Id, groupsList[i].Name, groupsList[i].DeviceCount, groupsList[i].JoinCode)
	}

	tbl.Print()
}
