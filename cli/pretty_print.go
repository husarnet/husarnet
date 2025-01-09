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

func prettyPrintGroups(groups types.Groups) {
	table := Table{}
	table.SetTitle("Your groups")
	table.SetHeader("ID", "Emoji", "Name", "Device#")

	for _, group := range groups {
		table.AddRow(group.Id, group.Emoji, group.Name, pterm.Sprintf("%d", len(group.Devices)))
	}

	table.Println()
}

func prettyPrintGroup(group types.Group) {
	fmt.Println(group.Emoji, group.Name)
	fmt.Println()

	table := Table{}
	table.SetTitle("Devices in this group")
	table.SetHeader("IPv6", "Emoji", "Hostname")

	for _, dev := range group.Devices {
		table.AddRow(dev.Ip, dev.Emoji, dev.Hostname)
	}

	table.Println()
}

func prettyPrintDevices(devices types.Devices) {
	table := Table{}
	table.SetTitle("Devices in this group")
	table.SetHeader("IPv6", "Emoji", "Hostname")

	for _, dev := range devices {
		table.AddRow(dev.Ip, dev.Emoji, dev.Hostname)
	}

	table.Println()
}

func prettyPrintDevice(dev types.Device) {
	fmt.Println(dev.Emoji, dev.Hostname)
	fmt.Println("ip of this device:", dev.Ip)
	fmt.Println("known as:", dev.Hostname, strings.Join(dev.Aliases, ", "))
	fmt.Println("User agent:", dev.UserAgent)
	fmt.Println("Husarnet version installed:", dev.Version)
	fmt.Println("Last contact:", dev.LastContact)
}
