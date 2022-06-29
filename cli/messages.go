// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

const version string = "v0.2"

const usage string = "Husarnet Dashboard CLI, " + version + "\n" +
	`Manage your Husarnet groups and devices from your terminal instead of web frontend.

Usage: hdm <SUBCOMMAND> <SUBCOMMAND OPTIONS>

Subcommands:
  group - manage your Husarnet groups, eg. see your groups, create, rename, delete
  device - manage your Husarnet devices, eg. rename, delete or see to which groups is the device in 
  login - obtain short-lived API token to authenticate while executing queries
  help - display this help
  version - display version of the CLI.

Run 'hdm <SUBCOMMAND>' to see what options are available under subcommands.

Frequently used:
  hdm group ls - list all groups created on your account along with their joincodes
  hdm group create foo - create new Husarnet group named foo
  hdm device ls - list all your devices

In order to query against our API, you will need to authenticate first with your Dashboard account credentials. You can do it explicitly, via 'hdm login', but this can be skipped, because the program will ask you for credentials whenever you will try to query the API unauthenticated.

After successful login, an auth token is created and saved on your device. The token is valid for 30 minutes, so you don't need to login again on subsequent queries. Any query you execute gives you a new token, so the timer is reset with every command that uses the API. The program will prompt again in case your token expires.

For detailed introduction to the CLI and more examples, see https://husarnet.com/docs
`

const groupSubcommandUsage string = `hdm group - Husarnet group management. Available commands:

  hdm group list - list your groups
  hdm group create <NAME> - create group named <NAME>
  hdm group rename <group_ID> <NEW_NAME> - rename a group
  hdm group remove <group_ID> - remove a group. Will ask for confirmation.

Aliases:
  hdm groups - alias for 'hdm group list'
  hdm group ls - alias for 'hdm group list'
  hdm group rm - alias for 'hdm group remove'

If creating/renaming a group with a name containing spaces, use " " to wrap the name.
`

const deviceSubcommandUsage string = `hdm device - Manage your Husarnet devices. Available commands:

  hdm device list - list your devices 
  hdm device rename
  hdm device remove

Aliases:
  hdm devices - alias for 'hdm device list'
  hdm device ls - alias for 'hdm device list'
`

const credentialsPrompt string = "Please provide your credentials to Husarnet Dashboard."
const removeGroupConfirmationPrompt string = "Are you sure you want to delete this group? All the devices inside the group will be deleted as well."

const notEnoughArgsForCreateGroup string = "Not enough arguments for: group create. Provide a name. Example: group create foo"
const notEnoughArgsForRemoveGroup string = "Not enough arguments for: group remove. Provide an ID. Example: group remove 3456"
const notEnoughArgsForRenameGroup string = "Not enough arguments for: group rename. Provide an ID and new name for the group. Example: group rename 3456 better_named_net"
const notEnoughArgsForUnjoin string = "Not enough arguments for: group unjoin. Provide group ID as first argument and device IPv6 fragment as the second. Example: group unjoin 3456 beef"
const notEnoughArgsForShowGroup string = "Not enough arguments for: group show. Provide numeric ID of the group. Example: group show 3456"
const notEnoughArgsForRenameDevice string = "Not enough arguments for: device rename. Provide an IPv6 (fc94...) and new name for the device. Example: device rename fc94:dead:beef:1234:9b8d:8beb:9999:8888 foodev"
