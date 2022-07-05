// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

// Tag for code generator. Do not delete.
//go:generate go run github.com/Khan/genqlient

import (
	"fmt"
	"hdm/handlers"
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
				Name:  "version",
				Usage: "print the version of the CLI and also of the daemon, if available",
				Action: func(c *cli.Context) error {
					fmt.Println("CLI version: " + version)
					// TODO ask daemon
					return nil
				},
			},
			{
				Name:  "status",
				Usage: "Display current connectivity status",
				Action: func(c *cli.Context) error {
					callDaemonGet("/control/status")
					return nil
				},
			},
			{
				Name:  "join",
				Usage: "Connect to Husarnet group with given join code and with specified hostname",
				Action: func(c *cli.Context) error {
					// up to two params
					if c.Args().Len() < 1 {
						die("you need to provide joincode")
					}
					joincode := c.Args().Get(0)
					hostname := ""

					if c.Args().Len() == 2 {
						hostname = c.Args().Get(1)
					}

					params := url.Values{
						"code":     {joincode},
						"hostname": {hostname},
					}
					addDaemonApiSecret(&params)
					callDaemonPost("/control/join", params)
					return nil
				},
			},
			{
				Name:  "setup-server",
				Usage: "Connect your Husarnet device to different Husarnet infrastructure",
				Action: func(c *cli.Context) error {
					if c.Args().Len() < 1 {
						fmt.Println("you need to provide address of the dashboard")
						return nil
					}

					domain := c.Args().Get(0)

					params := url.Values{
						"domain": {domain},
					}
					addDaemonApiSecret(&params)
					callDaemonPost("/control/change-server", params)
					return nil
				},
			},
			{
				Name:  "whitelist",
				Usage: "Manage whitelist on the device.",
				Subcommands: []*cli.Command{
					{
						Name:  "ls",
						Usage: "list entries on the whitelist",
						Action: func(c *cli.Context) error {
							callDaemonGet("/control/whitelist/ls")
							return nil
						},
					},
					{
						Name:  "add",
						Usage: "add a device to your whitelist by Husarnet address",
						Action: func(c *cli.Context) error {
							if c.Args().Len() < 1 {
								fmt.Println("you need to provide Husarnet address of the device")
								return nil
							}
							addr := c.Args().Get(0)

							params := url.Values{
								"address": {addr},
							}
							addDaemonApiSecret(&params)
							callDaemonPost("/control/whitelist/add", params)
							return nil
						},
					},
					{
						Name:  "rm",
						Usage: "remove device from the whitelist",
						Action: func(c *cli.Context) error {
							if c.Args().Len() < 1 {
								fmt.Println("you need to provide Husarnet address of the device")
								return nil
							}
							addr := c.Args().Get(0)

							params := url.Values{
								"address": {addr},
							}
							addDaemonApiSecret(&params)
							callDaemonPost("/control/whitelist/rm", params)
							return nil
						},
					},
				},
			},
			{
				Name:  "dashboard",
				Usage: "Talk to Dashboard API and manage your devices and groups without using web frontend.",
				Subcommands: []*cli.Command{
					{
						Name:  "login",
						Usage: "obtain short-lived API token to authenticate while executing queries",
						Action: func(c *cli.Context) error {
							loginAndSaveAuthToken()
							return nil
						},
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
								Action: func(c *cli.Context) error {
									callAPI(handlers.ListGroupsHandler{})
									return nil
								},
							},
							{
								Name:  "show",
								Usage: "display a table of devices in a given group",
								Action: func(c *cli.Context) error {
									if c.Args().Len() < 1 {
										fmt.Println(notEnoughArgsForShowGroup)
										return nil
									}
									callAPI(handlers.ShowGroupHandler{}, c.Args().First())
									return nil
								},
							},
							{
								Name:  "unjoin",
								Usage: "remove given device from the given group. First arg is group ID and second is the fragment of device IPv6",
								Action: func(c *cli.Context) error {
									if c.Args().Len() < 2 {
										fmt.Println(notEnoughArgsForUnjoin)
										return nil
									}
									callAPI(handlers.UnjoinDeviceHandler{}, c.Args().Get(0), c.Args().Get(1))
									return nil
								},
							},
							{
								Name:    "create",
								Aliases: []string{"add"},
								Usage:   "create new group with a name given as an argument",
								Action: func(c *cli.Context) error {
									if c.Args().Len() < 1 {
										fmt.Println(notEnoughArgsForCreateGroup)
										return nil
									}
									callAPI(handlers.CreateGroupHandler{}, c.Args().First())
									return nil
								},
							},
							{
								Name:  "rename",
								Usage: "change name for group with id [ID] to [new name]",
								Action: func(c *cli.Context) error {
									if c.Args().Len() < 2 {
										fmt.Println(notEnoughArgsForRenameGroup)
										return nil
									}
									callAPI(handlers.RenameGroupHandler{}, c.Args().Get(0), c.Args().Get(1))
									return nil
								},
							},
							{
								Name:    "remove",
								Aliases: []string{"rm"},
								Usage:   "remove the group with given ID. Will ask for confirmation.",
								Action: func(c *cli.Context) error {
									if c.Args().Len() < 1 {
										fmt.Println(notEnoughArgsForRemoveGroup)
										return nil
									}
									askForConfirmation(removeGroupConfirmationPrompt)
									callAPI(handlers.RemoveGroupHandler{}, c.Args().First())
									return nil
								},
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
								Action: func(c *cli.Context) error {
									callAPI(handlers.ListDevicesHandler{})
									return nil
								},
							},
							{
								Name:  "rename",
								Usage: "change displayed name of a device",
								Action: func(c *cli.Context) error {
									if c.Args().Len() < 2 {
										fmt.Println(notEnoughArgsForRenameDevice)
										return nil
									}
									callAPI(handlers.RenameDeviceHandler{}, c.Args().Get(0), c.Args().Get(1))
									return nil
								},
							},
							{
								Name:    "remove",
								Aliases: []string{"rm"},
								Usage:   "remove device from your account",
								Action: func(c *cli.Context) error {
									notImplementedYet()
									return nil
								},
							},
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
