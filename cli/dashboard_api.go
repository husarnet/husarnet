// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"

	"github.com/Khan/genqlient/graphql"
)

func getDashboardUrl() string {
	return fmt.Sprintf("https://%s/graphql", husarnetDashboardFQDN)
}

func callDashboardAPI[responseType any](executeRequest func(client graphql.Client) (responseType, error)) responseType {
	token := getAuthToken()

	const spinnerMessage = "Executing an API call…"
	spinner := getSpinner(spinnerMessage, false)

	client := makeAuthenticatedClient(token)
	response, err := executeRequest(client)

	if isSignatureExpiredOrInvalid(err) {
		spinner.Fail("Invalid/expired token")
		username, password := getUserCredentialsFromStandardInput()
		token = loginAndSaveAuthToken(username, password)

		spinner = getSpinner(spinnerMessage, false)
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
