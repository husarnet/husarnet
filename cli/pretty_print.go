// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"github.com/husarnet/husarnet/cli/v2/types"
	"github.com/pterm/pterm"
	"strings"
)

func prettyPrintGroups(response *types.ApiResponse[types.Groups]) {
	table := Table{}
	table.SetTitle("Your groups")
	table.SetHeader("ID", "Emoji", "Name", "Device#")

	for _, group := range response.Payload {
		table.AddRow(group.Id, group.Emoji, group.Name, pterm.Sprintf("%d", len(group.Devices)))
	}

	table.Println()
}

func prettyPrintGroup(response *types.ApiResponse[types.Group]) {
	fmt.Println(response.Payload.Emoji, response.Payload.Name)
	fmt.Println()

	table := Table{}
	table.SetTitle("Devices in this group")
	table.SetHeader("IPv6", "Emoji", "Hostname")

	for _, dev := range response.Payload.Devices {
		table.AddRow(dev.Ip, dev.Emoji, dev.Hostname)
	}

	table.Println()
}

func prettyPrintGroupDetails(response *types.ApiResponse[types.GroupDetails]) {
	fmt.Println(response.Payload.Group.Emoji, "", response.Payload.Group.Name)
	fmt.Println()

	table := Table{}
	table.SetTitle("Devices in this group")
	table.SetHeader("IPv6", "Emoji", "Hostname")

	for _, dev := range response.Payload.Group.Devices {
		table.AddRow(dev.Ip, dev.Emoji, dev.Hostname)
	}

	table.Println()
}

func prettyPrintDevices(response *types.ApiResponse[types.Devices]) {
	table := Table{}
	table.SetTitle("Devices in this group")
	table.SetHeader("IPv6", "Emoji", "Hostname")

	for _, dev := range response.Payload {
		table.AddRow(dev.Ip, dev.Emoji, dev.Hostname)
	}

	table.Println()
}

func prettyPrintDevice(response *types.ApiResponse[types.Device]) {
	fmt.Println(response.Payload.Emoji, response.Payload.Hostname)
	fmt.Println("ip of this device:", response.Payload.Ip)
	fmt.Println("known as:", response.Payload.Hostname, strings.Join(response.Payload.Aliases, ", "))
	fmt.Println("User agent:", response.Payload.UserAgent)
	fmt.Println("Husarnet version installed:", response.Payload.Version)
	fmt.Println("Last contact:", response.Payload.LastContact)
}
