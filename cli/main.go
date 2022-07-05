// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

// Tag for code generator. Do not delete.
//go:generate go run github.com/Khan/genqlient

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"

	"github.com/Khan/genqlient/graphql"
	"github.com/urfave/cli/v2"
)

// Global TODO:
// allow for removal/update via name, not an ID
// GET transport method for queries (defaults to POST)
// some user friendliness, validating input e.g. we expected number here

type handler interface {
	PerformRequest(client graphql.Client, args ...string) (interface{}, error)
	PrintResults(data interface{})
}

var graphqlServerURL string
var husarnetDaemonURL string = "http://127.0.0.1:16216"
var verboseLogs bool

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

func main() {
	app := &cli.App{
		Name:     "Husarnet CLI",
		HelpName: "husarnet",
		Usage:    "Manage your Husarnet groups and devices from your terminal",
		Flags: []cli.Flag{
			&cli.StringFlag{
				Name:        "url",
				Aliases:     []string{"u"},
				Value:       "https://app.husarnet.com/graphql",
				Usage:       "URL for your GraphQL endpoint. Remember to include scheme in this!",
				EnvVars:     []string{"HUSARNET_GRAPHQL_URL"},
				Destination: &graphqlServerURL,
			},
			&cli.BoolFlag{
				Name:        "verbose",
				Aliases:     []string{"v"},
				Usage:       "Show verbose logs (for debugging purposes)",
				Destination: &verboseLogs,
			},
		},
		Commands: []*cli.Command{
			{
				Name:   "version",
				Usage:  "print the version of the CLI and also of the daemon, if available",
				Action: handleVersion,
			},
			{
				Name:   "status",
				Usage:  "Display current connectivity status",
				Action: handleStatus,
			},
			{
				Name:   "join",
				Usage:  "Connect to Husarnet group with given join code and with specified hostname",
				Action: handleJoin,
			},
			{
				Name:   "setup-server",
				Usage:  "Connect your Husarnet device to different Husarnet infrastructure",
				Action: handleSetupServer,
			},
			{
				Name:  "whitelist",
				Usage: "Manage whitelist on the device.",
				Subcommands: []*cli.Command{
					{
						Name:   "ls",
						Usage:  "list entries on the whitelist",
						Action: handleWhitelistLs,
					},
					{
						Name:   "add",
						Usage:  "add a device to your whitelist by Husarnet address",
						Action: handleWhitelistAdd,
					},
					{
						Name:   "rm",
						Usage:  "remove device from the whitelist",
						Action: handleWhitelistRm,
					},
				},
			},
			{
				Name:  "dashboard",
				Usage: "Talk to Dashboard API and manage your devices and groups without using web frontend.",
				Subcommands: []*cli.Command{
					{
						Name:   "login",
						Usage:  "obtain short-lived API token to authenticate while executing queries",
						Action: handleLogin,
					},
					{
						Name:     "group",
						Usage:    "Husarnet group management, eg. see your groups, create, rename, delete",
						Category: "dashboard",
						Subcommands: []*cli.Command{
							{
								Name:    "list",
								Aliases: []string{"ls"},
								Usage:   "display a table of all your groups (with summary information)",
								Action:  handleGroupLs,
							},
							{
								Name:   "show",
								Usage:  "display a table of devices in a given group",
								Action: handleGroupShow,
							},
							{
								Name:   "unjoin",
								Usage:  "remove given device from the given group. First arg is group ID and second is the fragment of device IPv6",
								Action: handleGroupUnjoin,
							},
							{
								Name:    "create",
								Aliases: []string{"add"},
								Usage:   "create new group with a name given as an argument",
								Action:  handleGroupCreate,
							},
							{
								Name:   "rename",
								Usage:  "change name for group with id [ID] to [new name]",
								Action: handleGroupRename,
							},
							{
								Name:    "remove",
								Aliases: []string{"rm"},
								Usage:   "remove the group with given ID. Will ask for confirmation.",
								Action:  handleGroupRemove,
							},
						},
					},
					{
						Name:     "device",
						Usage:    "Husarnet device management",
						Category: "dashboard",
						Subcommands: []*cli.Command{
							{
								Name:    "list",
								Aliases: []string{"ls"},
								Usage:   "display a table of all your devices and which groups are they in",
								Action:  handleDeviceLs,
							},
							{
								Name:   "rename",
								Usage:  "change displayed name of a device",
								Action: handleDeviceRename,
							},
							{
								Name:    "remove",
								Aliases: []string{"rm"},
								Usage:   "remove device from your account",
								Action:  handleDeviceRemove},
						},
					},
				},
			},
		},
	}

	err := app.Run(os.Args)
	if err != nil {
		log.Fatal(err)
	}
}
