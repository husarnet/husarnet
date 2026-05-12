// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"net/netip"
	"net/url"
	"os"
	"syscall"

	"github.com/husarnet/husarnet/cli/v2/config"
	"github.com/husarnet/husarnet/cli/v2/types"

	"github.com/pterm/pterm"
)

func getDashboardPeerInfoByHostname(s types.DaemonStatus, hostname string) *types.DashboardPeerInfo {
	// returns first matching one.
	for _, dashboardPeerInfo := range s.Config.Dashboard.Peers {
		if dashboardPeerInfo.Hostname == hostname {
			return &dashboardPeerInfo
		}
		for _, alias := range dashboardPeerInfo.Aliases {
			if alias == hostname {
				return &dashboardPeerInfo
			}
		}
	}
	return nil
}

func getDaemonPeerInfoByAddr(s types.DaemonStatus, addr netip.Addr) *types.DaemonPeerInfo {
	for _, peer := range s.LiveData.Peers {
		if peer.Address == addr {
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

func getFullPath(file string) string {
	if onWindows() {
		sep := string(os.PathSeparator)
		return os.ExpandEnv("${programdata}") + sep + "husarnet" + sep + file
	}
	return "/var/lib/husarnet/" + file
}

func getApiErrorString[ResultType any](response types.DaemonResponse[ResultType]) string {
	if response.Status == "success" {
		return "status ok/no error"
	}

	return pterm.Sprintf("response status: %s, error: %s", response.Status, response.Error)
}

func handlePotentialDaemonApiRequestError[ResultType any](response types.DaemonResponse[ResultType], err error) {
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

func callDaemonRetryable[ResultType any](retryable bool, route string, urlencodedBody url.Values, lambda func(route string, urlencodedBody url.Values) (types.DaemonResponse[ResultType], error)) (types.DaemonResponse[ResultType], error) {
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
func callDaemonGetRaw[ResultType any](retryable bool, route string) (types.DaemonResponse[ResultType], error) {
	return callDaemonRetryable(retryable, route, url.Values{}, func(route string, urlencodedBody url.Values) (types.DaemonResponse[ResultType], error) {
		response, err := http.Get(config.GetDaemonApiUrl() + route)
		if err != nil {
			return types.DaemonResponse[ResultType]{}, err
		}

		return readResponse[types.DaemonResponse[ResultType]](response), nil
	})
}

func callDaemonGet[ResultType any](route string) types.DaemonResponse[ResultType] {
	response, err := callDaemonGetRaw[ResultType](true, route)
	handlePotentialDaemonApiRequestError(response, err)
	return response
}

// Those will always require an auth token
func callDaemonPostRaw[ResultType any](retryable bool, route string, urlencodedBody url.Values) (types.DaemonResponse[ResultType], error) {
	return callDaemonRetryable(retryable, route, urlencodedBody, func(route string, urlencodedBody url.Values) (types.DaemonResponse[ResultType], error) {
		response, err := http.PostForm(config.GetDaemonApiUrl()+route, urlencodedBody)
		if err != nil {
			return types.DaemonResponse[ResultType]{}, err
		}

		return readResponse[types.DaemonResponse[ResultType]](response), nil
	})
}

func callDaemonPost[ResultType any](route string, urlencodedBody url.Values) types.DaemonResponse[ResultType] {
	addDaemonApiSecret(&urlencodedBody)
	response, err := callDaemonPostRaw[ResultType](true, route, urlencodedBody)
	handlePotentialDaemonApiRequestError(response, err)
	return response
}

func getDaemonStatusRaw(retryable bool) (types.DaemonResponse[types.DaemonStatus], error) {
	return callDaemonGetRaw[types.DaemonStatus](retryable, "/api/status")
}
func getDaemonStatus() types.DaemonStatus {
	return callDaemonGet[types.DaemonStatus]("/api/status").Result
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
