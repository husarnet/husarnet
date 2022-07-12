// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"hdm/generated"
	"net/http"
	"os"
	"runtime"
	"strings"

	"github.com/Khan/genqlient/graphql"
)

type authedTransport struct {
	token   string
	wrapped http.RoundTripper
}

func (t *authedTransport) RoundTrip(req *http.Request) (*http.Response, error) {
	req.Header.Set("Authorization", "JWT "+t.token)
	return t.wrapped.RoundTrip(req)
}

func getTokenFilePath() string {
	if runtime.GOOS == "windows" {
		sep := string(os.PathSeparator)
		return os.ExpandEnv("${%localappdata%}") + sep + "Temp" + sep + "hsrnet-webtoken"
	}
	return "/tmp/hsrnet-webtoken"
}

func makeAuthenticatedClient(authToken string) graphql.Client {
	return graphql.NewClient(getDashboardUrl(),
		&http.Client{Transport: &authedTransport{token: authToken, wrapped: http.DefaultTransport}})
}

func saveAuthTokenToFile(authToken string) {
	// the token could possibly be stored in /var/lib/husarnet
	// but that's not ideal, since it would imply the need for sudo before each command
	// TODO solve this.
	writeFileErr := os.WriteFile(getTokenFilePath(), []byte(authToken), 0644)

	if writeFileErr != nil {
		fmt.Println("Error: could not save the auth token. " + writeFileErr.Error())
	}

	logV("Saving token", authToken)
}

func loginAndSaveAuthToken() string {
	username, password := getUserCredentialsFromStandardInput()
	authClient := graphql.NewClient(getDashboardUrl(), http.DefaultClient)
	tokenResp, tokenErr := generated.ObtainToken(authClient, username, password)
	if tokenErr != nil {
		fmt.Println("Authentication error occured.")
		die(tokenErr.Error())
	}
	token := tokenResp.TokenAuth.Token
	saveAuthTokenToFile(token)
	return token
}

func getAuthToken() string {
	tokenFromFile, err := os.ReadFile(getTokenFilePath())
	if err == nil {
		logV("Found token in file!")
		return string(tokenFromFile)
	}
	newToken := loginAndSaveAuthToken()
	return newToken
}

func getRefreshedToken(authToken string) string {
	client := graphql.NewClient(getDashboardUrl(),
		&http.Client{Transport: &authedTransport{token: authToken, wrapped: http.DefaultTransport}})
	resp, err := generated.RefreshToken(client, authToken)
	if err != nil {
		panic(err)
	}
	return resp.RefreshToken.Token
}

func refreshToken(authToken string) {
	refreshedToken := getRefreshedToken(authToken)
	saveAuthTokenToFile(refreshedToken)
}

func isSignatureExpiredOrInvalid(err error) bool {
	if err == nil {
		return false
	}
	message := err.Error()
	if strings.Contains(message, "Signature has expired") {
		logV("Signature has expired, user will need to provide credentials")
		return true
	}
	if strings.Contains(message, "Error decoding signature") {
		logV("JWT Token signature is invalid. This may happen when user changed the endpoint URL in the meantime.")
		return true
	}
	die("Fatal: unknown error from the server: " + message)
	return true
}
