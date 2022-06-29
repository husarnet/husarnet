// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

// Tag for code generator. Do not delete.
//go:generate go run github.com/Khan/genqlient

import (
	"fmt"
	"hdm/handlers"
	"log"
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
func callDaemon() {

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
				Value:       "https://aws-1.husarnet.com/graphql",
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
					fmt.Println("0.3")
					return nil
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
