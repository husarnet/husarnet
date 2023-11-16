// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"net/netip"
	"strings"
	"time"

	"github.com/pterm/pterm"
)

func waitAction(message string, lambda func(*DaemonStatus) (bool, string)) error {
	spinner := getSpinner(message, true)
	observer := NewDumbObserver(func(old, new string) {
		spinner.UpdateText(new)
	})

	failedCounter := 0
	for {
		failedCounter++
		if failedCounter > 120 {
			spinner.Fail("timeout")
			return fmt.Errorf("timeout")
		}

		response, err := getDaemonStatusRaw(false)
		if err != nil {
			time.Sleep(time.Second)
			continue
		}

		success, temporaryMessage := lambda(&response.Result)
		if success {
			observer.Update(message)
			spinner.Success()
			return nil
		}

		observer.Update(message + " " + temporaryMessage)

		time.Sleep(time.Second)
	}
}

func waitDaemon() error {
	return waitAction("Waiting until we can communicate with husarnet daemon…", func(status *DaemonStatus) (bool, string) { return true, "" })
}

func waitBaseANY() error {
	return waitAction("Waiting for Base server connection (any protocol)…", func(status *DaemonStatus) (bool, string) {
		return status.BaseConnection.Type == "TCP" || status.BaseConnection.Type == "UDP", ""
	})
}

func waitBaseUDP() error {
	return waitAction("Waiting for Base server connection (UDP)…", func(status *DaemonStatus) (bool, string) { return status.BaseConnection.Type == "UDP", "" })
}

func waitWebsetup() error {
	return waitAction("Waiting for websetup connection…", func(status *DaemonStatus) (bool, string) { return status.ConnectionStatus["websetup"], "" })
}

func waitJoined() error {
	return waitAction("Waiting until the device is joined…", func(status *DaemonStatus) (bool, string) { return status.IsJoined, "" })
}

func waitHost(hostnameOrIp string) error {
	hostname := ""
	addr, err := netip.ParseAddr(hostnameOrIp)
	if err != nil {
		hostname = hostnameOrIp
	}

	printInfo("Remember that in order to consider a connection established there need to be at least 1 attempt of connection using the regular networking stack - i.e. a single ping to a given host")

	return waitAction(pterm.Sprintf("Waiting until there's a connection to %s…", hostnameOrIp), func(status *DaemonStatus) (bool, string) {
		hostIp, present := status.HostTable[hostname]
		if hostname != "" && present {
			addr = hostIp
		}

		if !addr.IsValid() {
			return false, "peer addr is not known yet" // Multiple paths - invalid addr + host not present in host table is the most important one
		}

		peer := status.getPeerByAddr(addr)

		if peer == nil {
			return false, "unable to find peer in the peer list" // Unable to find peer
		}

		if !peer.IsActive {
			return false, "peer is not active yet" // Theres is no connection established yet (remember to try connecting to it though)
		}

		if !peer.IsSecure {
			return false, "secure connection has not yet been established"
		}

		return true, ""
	})
}

func waitHostnames(hostnames []string) error {
	return waitAction(pterm.Sprintf("Waiting until the following hostnames are known to: %s…", strings.Join(hostnames, ", ")), func(status *DaemonStatus) (bool, string) {
		for _, hostname := range hostnames {
			_, present := status.HostTable[hostname]
			if !present {
				return false, pterm.Sprintf("%s is unavailable", hostname)
			}
		}

		return true, ""
	})
}
