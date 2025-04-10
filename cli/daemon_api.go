// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"encoding/json"
	"errors"
	"github.com/husarnet/husarnet/cli/v2/config"
	"io"
	"net/http"
	"net/netip"
	"net/url"
	"os"
	"syscall"

	"github.com/pterm/pterm"
)

type DaemonResponse[ResultType any] struct {
	Status string
	Error  string
	Result ResultType
}

func (response *DaemonResponse[ResultType]) IsOk() bool {
	return response.Status == "success" || response.Status == "ok"
}

type EmptyResult interface{}

type BaseConnectionStatus struct {
	Address netip.Addr
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

	// TODO reenable this after latency support is readded
	// LatencyMs int `json:"latency_ms"` // TODO long-term figure out how to convert it to time.Duration

	SourceAddresses   []netip.AddrPort `json:"source_addresses"`
	TargetAddresses   []netip.AddrPort `json:"target_addresses"`
	UsedTargetAddress netip.AddrPort   `json:"used_target_address"`
}

type DaemonLiveData struct {
	BaseConnection BaseConnectionStatus `json:"base_connection"`
}

type ApiConfig struct {
	IsClaimed bool `json:"is_claimed"`
}

type DaemonConfig struct {
	Api ApiConfig `json:"api_config"`
}

type DaemonStatus struct {
	Version       string `json:"version"`
	UserAgent     string `json:"user_agent"`
	DashboardFQDN string `json:"dashboard_fqdn"`
	HooksEnabled  bool   `json:"hooks_enabled"`

	WebsetupAddress netip.Addr     `json:"websetup_address"`
	LiveData        DaemonLiveData `json:"live"`
	Config          DaemonConfig   `json:"config"`
	LocalIP         netip.Addr     `json:"local_ip"`
	LocalHostname   string         `json:"local_hostname"`

	IsJoined         bool            `json:"is_joined"`
	IsReady          bool            `json:"is_ready"`
	IsReadyToJoin    bool            `json:"is_ready_to_join"`
	ConnectionStatus map[string]bool `json:"connection_status"`

	Whitelist    []netip.Addr
	UserSettings map[string]string     `json:"user_settings"` // TODO long-term - think about a better structure (more importantly enums) to hold this if needed
	HostTable    map[string]netip.Addr `json:"host_table"`
	Peers        []PeerStatus
}

func (s DaemonStatus) getPeerByAddr(addr netip.Addr) *PeerStatus {
	for _, peer := range s.Peers {
		if peer.HusarnetAddress == addr {
			return &peer
		}
	}

	return nil
}

func addDaemonApiSecret(params *url.Values) {
	overriddenSecret := config.GetDaemonApiSecret()
	if len(overriddenSecret) > 0 {
		params.Add("secret", overriddenSecret)
		return
	}

	apiSecret, err := os.ReadFile(config.GetDaemonApiSecretPath())
	if err != nil {
		printError("Error reading secret file, are you root/administrator? " + err.Error())
		rerunWithSudoOrDie()
	}

	params.Add("secret", string(apiSecret))
}

func getDaemonIdPath() string {
	if onWindows() {
		sep := string(os.PathSeparator)
		return os.ExpandEnv("${programdata}") + sep + "husarnet" + sep + "id"
	}
	return "/var/lib/husarnet/id"
}

func getApiErrorString[ResultType any](response DaemonResponse[ResultType]) string {
	if response.Status == "success" {
		return "status ok/no error"
	}

	return pterm.Sprintf("response status: %s, error: %s", response.Status, response.Error)
}

func handlePotentialDaemonApiRequestError[ResultType any](response DaemonResponse[ResultType], err error) {
	if err != nil {
		dieE(err)
	}

	if !response.IsOk() {
		die(getApiErrorString(response))
	}
}

func readResponse[ResponseType any](resp *http.Response) ResponseType {
	body, err := io.ReadAll(resp.Body)
	defer resp.Body.Close()
	if err != nil {
		dieE(err)
	}

	printDebug(string(body))

	var response ResponseType
	err = json.Unmarshal(body, &response)
	if err != nil {
		dieE(err)
	}
	return response
}

func callDaemonRetryable[ResultType any](retryable bool, route string, urlencodedBody url.Values, lambda func(route string, urlencodedBody url.Values) (DaemonResponse[ResultType], error)) (DaemonResponse[ResultType], error) {
	response, err := lambda(route, urlencodedBody)
	if err != nil {
		printDebug("%v", err)

		if retryable && errors.Is(err, syscall.ECONNREFUSED) {
			printInfo("Daemon does not seem to be running")
			restartDaemonWithConfirmationPrompt()
			waitDaemon()
			return lambda(route, urlencodedBody)
		}
	}

	return response, err
}

// Technically those should not require auth tokens so can be run at any time
func callDaemonGetRaw[ResultType any](retryable bool, route string) (DaemonResponse[ResultType], error) {
	return callDaemonRetryable(retryable, route, url.Values{}, func(route string, urlencodedBody url.Values) (DaemonResponse[ResultType], error) {
		response, err := http.Get(config.GetDaemonApiUrl() + route)
		if err != nil {
			return DaemonResponse[ResultType]{}, err
		}

		return readResponse[DaemonResponse[ResultType]](response), nil
	})
}

func callDaemonGet[ResultType any](route string) DaemonResponse[ResultType] {
	response, err := callDaemonGetRaw[ResultType](true, route)
	handlePotentialDaemonApiRequestError(response, err)
	return response
}

// Those will always require an auth token
func callDaemonPostRaw[ResultType any](retryable bool, route string, urlencodedBody url.Values) (DaemonResponse[ResultType], error) {
	return callDaemonRetryable(retryable, route, urlencodedBody, func(route string, urlencodedBody url.Values) (DaemonResponse[ResultType], error) {
		response, err := http.PostForm(config.GetDaemonApiUrl()+route, urlencodedBody)
		if err != nil {
			return DaemonResponse[ResultType]{}, err
		}

		return readResponse[DaemonResponse[ResultType]](response), nil
	})
}

func callDaemonPost[ResultType any](route string, urlencodedBody url.Values) DaemonResponse[ResultType] {
	addDaemonApiSecret(&urlencodedBody)
	response, err := callDaemonPostRaw[ResultType](true, route, urlencodedBody)
	handlePotentialDaemonApiRequestError(response, err)
	return response
}

func getDaemonStatusRaw(retryable bool) (DaemonResponse[DaemonStatus], error) {
	return callDaemonGetRaw[DaemonStatus](retryable, "/api/status")
}
func getDaemonStatus() DaemonStatus {
	return callDaemonGet[DaemonStatus]("/api/status").Result
}

// isDaemonRunning returns true if daemon is returning sane response to /api/status
func isDaemonRunning() bool {
	response, err := getDaemonStatusRaw(false)
	return err == nil && response.IsOk()
}

func getDaemonRunningVersion() string {
	response, err := getDaemonStatusRaw(true)
	if err != nil {
		return pterm.Sprintf("Unknown (%s)", err)
	}
	if !response.IsOk() {
		return pterm.Sprintf("Unknown (%s)", getApiErrorString(response))
	}

	return response.Result.Version
}
