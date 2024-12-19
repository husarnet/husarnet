// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"syscall"
)

type ApiResponse[PayloadType any] struct {
	Type     string      `json:"type"`
	Errors   []string    `json:"errors"`
	Warnings []string    `json:"warnings"`
	Message  string      `json:"message"`
	Payload  PayloadType `json:"payload"`
}

type MaybeError struct {
	Error string `json:"error"`
}

func buildUrl(apiEndpoint string) string {
	urlValues := url.Values{}
	addDaemonApiSecret(&urlValues)
	return getDaemonApiUrl() + "/api/forward" + apiEndpoint + "?" + urlValues.Encode()
}

func performDashboardApiRequest[responsePayloadType any](request *http.Request) ApiResponse[responsePayloadType] {
	response, err := http.DefaultClient.Do(request)

	if err != nil {
		if errors.Is(err, syscall.ECONNREFUSED) {
			dieE(errors.New("daemon refused the connection; make sure it is running before trying again"))
		}
		dieE(err)
	}

	responseBytes, err := io.ReadAll(response.Body)
	defer response.Body.Close()
	if err != nil {
		dieE(err)
	}

	if response.StatusCode != http.StatusOK { // infra error
		// try figure out better error message
		var maybeError MaybeError
		marshalingErr := json.Unmarshal(responseBytes, &maybeError)
		if marshalingErr != nil {
			dieE(errors.New("API responded with " + response.Status))
		} else {
			dieE(errors.New("API responded with " + response.Status + ": " + maybeError.Error))
		}
	}

	var apiResponse ApiResponse[responsePayloadType]
	err = json.Unmarshal(responseBytes, &apiResponse)
	if err != nil {
		dieE(err)
	}

	return apiResponse
}

func printJsonOrError(data any) {
	jsonBytes, err := json.Marshal(data)
	if err != nil {
		// we will use this function only with data previously successfully unmarshalled
		// so this should never happen in practice; checking doesn't hurt though
		dieE(err)
	}
	fmt.Println(string(jsonBytes))
}

func callDashboardApi[responsePayloadType any](method, endpoint string) ApiResponse[responsePayloadType] {
	request, err := http.NewRequest(method, buildUrl(endpoint), nil)
	if err != nil {
		dieE(err)
	}
	// TODO: make cpp part respect this
	request.Header.Set("User-Agent", "Husarnet CLI version "+cliVersion)
	return performDashboardApiRequest[responsePayloadType](request)
}

func callDashboardApiWithInput[requestPayloadType, responsePayloadType any](method, endpoint string, jsonBody requestPayloadType) ApiResponse[responsePayloadType] {
	jsonBytes, err := json.Marshal(jsonBody)
	if err != nil {
		dieE(err)
	}
	fwdUrl := buildUrl(endpoint)
	request, err := http.NewRequest(method, fwdUrl, bytes.NewBuffer(jsonBytes))
	if err != nil {
		dieE(err)
	}
	return performDashboardApiRequest[responsePayloadType](request)
}
