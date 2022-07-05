// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"io"
	"net/http"
	"net/url"
	"os"

	"github.com/Khan/genqlient/graphql"
)

type handler interface {
	PerformRequest(client graphql.Client, args ...string) (interface{}, error)
	PrintResults(data interface{})
}

func callAPI(handler handler, args ...string) {
	token := getAuthToken()
	client := makeAuthenticatedClient(token)
	resp, err := handler.PerformRequest(client, args...)

	if isSignatureExpiredOrInvalid(err) {
		token = loginAndSaveAuthToken()
		client := makeAuthenticatedClient(token)
		resp, err = handler.PerformRequest(client, args...)
	}

	if err != nil {
		die("GraphQL server at " + graphqlServerURL + " returned an error: " + err.Error())
	}

	handler.PrintResults(resp)

	// after performing any request, we hit API again to refresh token thus prolong the session
	refreshToken(token)
}

// TODO: implement calling daemon, starting it if is not running yet, etc...
// Basic functions that just print responses
func callDaemonGet(route string) {
	resp, err := http.Get(husarnetDaemonURL + route)
	handlePotentialDaemonRequestError(err)
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	fmt.Println(string(body))
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

func callDaemonPost(route string, urlencodedBody url.Values) {
	resp, err := http.PostForm(husarnetDaemonURL+route, urlencodedBody)
	handlePotentialDaemonRequestError(err)
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	fmt.Println(string(body))
}

func notImplementedYet() {
	fmt.Println("Not implemented yet")
}
