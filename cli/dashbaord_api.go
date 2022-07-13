// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"

	"github.com/Khan/genqlient/graphql"
)

type handler interface {
	PerformRequest(client graphql.Client, args ...string) (interface{}, error)
	PrintResults(data interface{})
}

func getDashboardUrl() string {
	return fmt.Sprintf("https://%s/graphql", husarnetDashboardFQDN)
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
		die("GraphQL server at " + getDashboardUrl() + " returned an error: " + err.Error())
	}

	handler.PrintResults(resp)

	// after performing any request, we hit API again to refresh token thus prolong the session
	refreshToken(token)
}
