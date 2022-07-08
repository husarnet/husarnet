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
)

// TODO: implement calling daemon, starting it if is not running yet, etc...
// Basic functions that just print responses
func getDaemonApiUrl() string {
	return fmt.Sprintf("http://127.0.0.1:%d", husarnetDaemonApiPort)
}

func callDaemonGet(route string) {
	resp, err := http.Get(getDaemonApiUrl() + route)
	handlePotentialDaemonRequestError(err)
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		dieE(err)
	}

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
	resp, err := http.PostForm(getDaemonApiUrl()+route, urlencodedBody)
	handlePotentialDaemonRequestError(err)
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	fmt.Println(string(body))
}
