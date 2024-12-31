// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package requests

import (
	"bytes"
	"encoding/json"
	"errors"
	"github.com/husarnet/husarnet/cli/v2/config"
	"github.com/husarnet/husarnet/cli/v2/constants"
	"github.com/husarnet/husarnet/cli/v2/output"
	"github.com/husarnet/husarnet/cli/v2/types"
	"io"
	"net/http"
	"net/url"
	"os"
	"syscall"
)

// TODO: currently the same function is used elsewhere, move it to utils
func addDaemonApiSecret(params *url.Values) {
	overriddenSecret := config.GetDaemonApiSecret()
	if len(overriddenSecret) > 0 {
		params.Add("secret", overriddenSecret)
		return
	}

	apiSecret, err := os.ReadFile(config.GetDaemonApiSecretPath())
	if err != nil {
		output.DieWithError(errors.Join(errors.New("Error reading secret file, are you root/administrator? "), err))
	}

	params.Add("secret", string(apiSecret))
}

func buildUrl(apiEndpoint string) string {
	urlValues := url.Values{}
	addDaemonApiSecret(&urlValues)
	return config.GetDaemonApiUrl() + "/api/forward" + apiEndpoint + "?" + urlValues.Encode()
}

func performDashboardApiRequest[responsePayloadType any](request *http.Request) types.ApiResponse[responsePayloadType] {
	response, err := http.DefaultClient.Do(request)

	if err != nil {
		if errors.Is(err, syscall.ECONNREFUSED) {
			output.DieWithError(errors.New("daemon refused the connection; make sure it is running before trying again"))
		}
		output.DieWithError(err)
	}

	responseBytes, err := io.ReadAll(response.Body)
	defer response.Body.Close()
	if err != nil {
		output.DieWithError(err)
	}

	if response.StatusCode != http.StatusOK { // infra error
		// try figure out better error message
		var maybeError types.MaybeError
		marshalingErr := json.Unmarshal(responseBytes, &maybeError)
		if marshalingErr != nil {
			output.DieWithError(errors.New("API responded with " + response.Status))
		} else {
			output.DieWithError(errors.New("API responded with " + response.Status + ": " + maybeError.Error))
		}
	}

	var apiResponse types.ApiResponse[responsePayloadType]
	err = json.Unmarshal(responseBytes, &apiResponse)
	if err != nil {
		output.DieWithError(err)
	}

	return apiResponse
}

func callDashboardApi[responsePayloadType any](method, endpoint string) types.ApiResponse[responsePayloadType] {
	request, err := http.NewRequest(method, buildUrl(endpoint), nil)
	if err != nil {
		output.DieWithError(err)
	}
	// TODO: make cpp part respect this
	request.Header.Set("User-Agent", "Husarnet CLI version "+constants.Version)
	return performDashboardApiRequest[responsePayloadType](request)
}

func callDashboardApiWithInput[requestPayloadType, responsePayloadType any](method, endpoint string, jsonBody requestPayloadType) types.ApiResponse[responsePayloadType] {
	jsonBytes, err := json.Marshal(jsonBody)
	if err != nil {
		output.DieWithError(err)
	}
	fwdUrl := buildUrl(endpoint)
	request, err := http.NewRequest(method, fwdUrl, bytes.NewBuffer(jsonBytes))
	if err != nil {
		output.DieWithError(err)
	}
	return performDashboardApiRequest[responsePayloadType](request)
}
