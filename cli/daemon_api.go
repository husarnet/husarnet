// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/netip"
	"net/url"
	"os"
)

// TODO: implement calling daemon, starting it if is not running yet, etc...
// Basic functions that just print responses
func getDaemonApiUrl() string {
	return fmt.Sprintf("http://127.0.0.1:%d", husarnetDaemonAPIPort)
}

func handlePotentialDaemonRequestError(err error) {
	if err != nil {
		fmt.Println("something went wrong: " + err.Error())
		fmt.Println("Husarnet daemon is probably not running")
		// leak here?
		// try tu execute husarnet-daemon? via systemctl?
		os.Exit(1)
	}
}

func addDaemonApiSecret(params *url.Values) {
	apiSecret, err := os.ReadFile("/var/lib/husarnet/api_secret")
	if err != nil {
		die("Error reading secret file, are you root? " + err.Error())
	}
	params.Add("secret", string(apiSecret))
}

type DaemonResponse[ResultType any] struct {
	Status string
	Result ResultType
}

type EmptyResult struct{}

type BaseConnectionStatus struct {
	Address net.IP
	Port    int
	Type    string // TODO long-term - make it enum maybe
}

type PeerStatus struct {
	HusarnetAddress  netip.Addr
	LinkLocalAddress netip.AddrPort

	IsActive         bool
	IsReestablishing bool
	IsSecure         bool
	IsTunelled       bool

	LatencyMs int // TODO long-term figure out how to convert it to time.Duration

	SourceAddresses   []netip.AddrPort
	TargetAddresses   []netip.AddrPort
	UsedTargetAddress netip.AddrPort
}

type DaemonStatus struct {
	Version       string
	DashboardFQDN string

	WebsetupAddress netip.Addr
	BaseConnection  BaseConnectionStatus

	LocalIP       net.IP
	LocalHostname string

	IsJoined         bool
	IsReady          bool
	IsReadyToJoin    bool
	ConnectionStatus map[string]bool

	Whitelist    []netip.Addr
	UserSettings map[string]string // TODO long-term - think about a batter structure to hold this if needed
	HostTable    map[string]netip.Addr
	Peers        []PeerStatus
}

func readResponse[ResultType any](resp *http.Response) DaemonResponse[ResultType] {
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		dieE(err)
	}

	var response DaemonResponse[ResultType]
	err = json.Unmarshal([]byte(body), &response)
	if err != nil {
		dieE(err)
	}

	return response
}

// TODO add versions of those calls for getting plain json/string response
// so users can specify --json

func callDaemonGet[ResultType any](route string) DaemonResponse[ResultType] {
	resp, err := http.Get(getDaemonApiUrl() + route)
	handlePotentialDaemonRequestError(err)
	defer resp.Body.Close()

	return readResponse[ResultType](resp)
}

func callDaemonPost[ResultType any](route string, urlencodedBody url.Values) DaemonResponse[ResultType] {
	resp, err := http.PostForm(getDaemonApiUrl()+route, urlencodedBody)
	handlePotentialDaemonRequestError(err)
	defer resp.Body.Close()

	return readResponse[ResultType](resp)
}

func getDaemonStatus() DaemonStatus {
	response := callDaemonGet[DaemonStatus]("/control/status")
	return response.Result
}

func getDaemonRunningVersion() string {
	return getDaemonStatus().Version
}
