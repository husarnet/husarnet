// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package handlers

import (
	"hdm/generated"

	"github.com/Khan/genqlient/graphql"
	"github.com/rodaine/table"
)

type ShowGroupHandler struct{}

func (h ShowGroupHandler) PerformRequest(client graphql.Client, args ...string) (interface{}, error) {
	return generated.ShowGroup(client, args[0])
}

func (h ShowGroupHandler) PrintResults(data interface{}) {
	resp := data.(*generated.ShowGroupResponse)
	devicesList := resp.GroupMembersById

	tbl := table.New("ID(ipv6)", "Name", "Version", "UserAgent")

	for i := 0; i < len(devicesList); i++ {
		tbl.AddRow(devicesList[i].DeviceId, devicesList[i].Name, devicesList[i].Version, devicesList[i].UserAgent)
	}

	tbl.Print()
}
