// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"net/netip"
	"net/url"
	"os"
	"syscall"

	"github.com/mitchellh/go-wordwrap"
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

// This can be easily expanded upon to include aditinal data we may want to possibly return
type StandardResult struct {
	Notifications          []string `json:"notifications"`
	NotificationsEnabled   bool     `json:"notifications_enabled"`
	NotificationsToDisplay bool     `json:"notifications_to_display"`
	IsDirty                bool     `json:"is_dirty"`
}

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

type DaemonStatus struct {
	Version       string
	DashboardFQDN string         `json:"dashboard_fqdn"`
	StdResult     StandardResult `json:"standard_result"`
	HooksEnabled  bool           `json:"hooks_enabled"`

	WebsetupAddress netip.Addr           `json:"websetup_address"`
	BaseConnection  BaseConnectionStatus `json:"base_connection"`

	LocalIP       netip.Addr `json:"local_ip"`
	LocalHostname string     `json:"local_hostname"`

	IsJoined         bool            `json:"is_joined"`
	IsReady          bool            `json:"is_ready"`
	IsReadyToJoin    bool            `json:"is_ready_to_join"`
	ConnectionStatus map[string]bool `json:"connection_status"`

	Whitelist    []netip.Addr
	UserSettings map[string]string     `json:"user_settings"` // TODO long-term - think about a better structure (more importantly enums) to hold this if needed
	HostTable    map[string]netip.Addr `json:"host_table"`
	Peers        []PeerStatus
}

type LogsResponse struct {
	Logs      []string       `json:"logs"`
	StdResult StandardResult `json:"standard_result"`
}

type LogsSettings struct {
	VerbosityLevel int            `json:"verbosity"`
	Size           int            `json:"size"`
	CurrentSize    int            `json:"current_size"`
	StdResult      StandardResult `json:"standard_result"`
}

func (s DaemonStatus) getPeerByAddr(addr netip.Addr) *PeerStatus {
	for _, peer := range s.Peers {
		if peer.HusarnetAddress == addr {
			return &peer
		}
	}

	return nil
}

func getDaemonApiUrl() string {
	if husarnetDaemonAPIPort != 0 {
		return fmt.Sprintf("http://127.0.0.1:%d", husarnetDaemonAPIPort)
	}

	return fmt.Sprintf("http://127.0.0.1:%d", defaultDaemonAPIPort)
}

func getDaemonApiSecretPath() string {
	if onWindows() {
		sep := string(os.PathSeparator)
		return os.ExpandEnv("${programdata}") + sep + "husarnet" + sep + "daemon_api_token"
	}
	return "/var/lib/husarnet/daemon_api_token"
}

func addDaemonApiSecret(params *url.Values) {
	apiSecret, err := os.ReadFile(getDaemonApiSecretPath())
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

func readResponse[ResultType any](resp *http.Response) DaemonResponse[ResultType] {
	body, err := io.ReadAll(resp.Body)
	defer resp.Body.Close()
	if err != nil {
		dieE(err)
	}

	printDebug(string(body))

	var response DaemonResponse[ResultType]
	err = json.Unmarshal([]byte(body), &response)
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
		response, err := http.Get(getDaemonApiUrl() + route)
		if err != nil {
			return DaemonResponse[ResultType]{}, err
		}

		return readResponse[ResultType](response), nil
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
		response, err := http.PostForm(getDaemonApiUrl()+route, urlencodedBody)
		if err != nil {
			return DaemonResponse[ResultType]{}, err
		}

		return readResponse[ResultType](response), nil
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
	response, err := getDaemonStatusRaw(true)
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

func getDaemonsDashboardFqdn() string {
	response, err := getDaemonStatusRaw(false)
	if err != nil {
		return defaultDashboard
	}

	return response.Result.DashboardFQDN
}

func handleStandardResult(res StandardResult) {
	if res.IsDirty {
		formatter := extractFormatter(redDot)
		help := "Daemon's dirty flag is set. You need to restart husarnet-daemon in order to reflect the current settings (like the Dashboard URL)"
		pterm.Printfln("%s %s", redDot, formatter(help))
	}
	if (res.NotificationsEnabled) && (len(res.Notifications) > 0) && (res.NotificationsToDisplay) {
		for _, announcement := range res.Notifications {
			wrapped := wordwrap.WrapString(announcement, 60)
			pterm.Println(wrapped)
		}
	}
}
