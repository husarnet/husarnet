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
	"runtime"
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

func getDaemonApiSecretPath() string {
	if runtime.GOOS == "windows" {
		sep := string(os.PathSeparator)
		return os.ExpandEnv("${programdata}") + sep + "Husarnet" + sep + "api_secret"
	}
	return "/var/lib/husarnet/api_secret"
}

func addDaemonApiSecret(params *url.Values) {
	apiSecret, err := os.ReadFile(getDaemonApiSecretPath())
	if err != nil {
		die("Error reading secret file, are you root/administrator? " + err.Error())
	}
	params.Add("secret", string(apiSecret))
}

type DaemonResponse[ResultType any] struct {
	Status string
	Result ResultType
}

type EmptyResult interface{}

type BaseConnectionStatus struct {
	Address net.IP
	Port    int
	Type    string // TODO long-term - make it enum maybe
}

type PeerStatus struct {
	HusarnetAddress  netip.Addr     `json:"husarnet_address"`
	LinkLocalAddress netip.AddrPort `json:"link_local_address"`

	IsActive         bool `json:"is_active"`
	IsReestablishing bool `json:"is_reestablishing"`
	IsSecure         bool `json:"is_secure"`
	IsTunelled       bool `json:"is_tunelled"`

	LatencyMs int `json:"latency_ms"` // TODO long-term figure out how to convert it to time.Duration

	SourceAddresses   []netip.AddrPort `json:"source_addresses"`
	TargetAddresses   []netip.AddrPort `json:"target_addresses"`
	UsedTargetAddress netip.AddrPort   `json:"used_target_address"`
}

type DaemonStatus struct {
	Version       string
	DashboardFQDN string `json:"dashboard_fqdn"`

	WebsetupAddress netip.Addr           `json:"websetup_address"`
	BaseConnection  BaseConnectionStatus `json:"base_connection"`

	LocalIP       net.IP `json:"local_ip"`
	LocalHostname string `json:"local_hostname"`

	IsJoined         bool            `json:"is_joined"`
	IsReady          bool            `json:"is_ready"`
	IsReadyToJoin    bool            `json:"is_ready_to_join"`
	ConnectionStatus map[string]bool `json:"connection_status"`

	Whitelist    []netip.Addr
	UserSettings map[string]string     `json:"user_settings"` // TODO long-term - think about a better structure (more importantly enums) to hold this if needed
	HostTable    map[string]netip.Addr `json:"host_table"`
	Peers        []PeerStatus
}

func readResponse[ResultType any](resp *http.Response) DaemonResponse[ResultType] {
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		dieE(err)
	}

	if verboseLogs {
		fmt.Println(string(body))
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
