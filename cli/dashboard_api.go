// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"

	"github.com/Khan/genqlient/graphql"
	"github.com/pterm/pterm"
)

func getDashboardUrl() string {
	return fmt.Sprintf("https://%s/graphql", husarnetDashboardFQDN)
}

func callDashboardAPI[responseType any](executeRequest func(client graphql.Client) (responseType, error)) responseType {
	const spinnerMessage = "Executing an API call…"
	spinner, _ := pterm.DefaultSpinner.Start(spinnerMessage)

	token := getAuthToken()
	client := makeAuthenticatedClient(token)
	response, err := executeRequest(client)

	if isSignatureExpiredOrInvalid(err) {
		spinner.Fail("Invalid/expired token")
		token = loginAndSaveAuthToken()

		spinner, _ = pterm.DefaultSpinner.Start(spinnerMessage)
		client := makeAuthenticatedClient(token)
		response, err = executeRequest(client)
	}

	if err != nil {
		spinner.Fail(err)
		die("GraphQL server at " + getDashboardUrl() + " returned an error: " + err.Error())
	}

	spinner.Success()

	// after performing any request, we hit API again to refresh token thus prolong the session
	refreshToken(token)

	return response
}
