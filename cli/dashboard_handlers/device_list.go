// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package dashboard_handlers

import (
	"hdm/generated"
	"strings"

	"github.com/Khan/genqlient/graphql"
	"github.com/rodaine/table"
)

type ListDevicesHandler struct{}

func (h ListDevicesHandler) PerformRequest(client graphql.Client, args ...string) (interface{}, error) {
	return generated.GetDevices(client)
}

func (h ListDevicesHandler) PrintResults(data interface{}) {
	resp := data.(*generated.GetDevicesResponse)
	devicesList := resp.Devices

	tbl := table.New("ID(ipv6)", "Name", "Version", "UserAgent", "Groups containing device")

	for i := 0; i < len(devicesList); i++ {
		groups := ""
		for _, group := range devicesList[i].GroupMemberships {
			groups += group.Name
			groups += ", "
		}
		groups = strings.TrimSuffix(groups, ", ")
		tbl.AddRow(devicesList[i].DeviceId, devicesList[i].Name, devicesList[i].Version, devicesList[i].UserAgent, groups)
	}

	tbl.Print()
}
