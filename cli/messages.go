// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

const version string = "2022.08.18.3"

const credentialsPrompt string = "Please provide your credentials to Husarnet Dashboard."
const removeGroupConfirmationPrompt string = "Are you sure you want to delete this group? All the devices inside the group will be deleted as well."

const notEnoughArgsForCreateGroup string = "Not enough arguments for: group create. Provide a name. Example: group create foo"
const notEnoughArgsForRemoveGroup string = "Not enough arguments for: group remove. Provide an ID. Example: group remove 3456"
const notEnoughArgsForRenameGroup string = "Not enough arguments for: group rename. Provide an ID and new name for the group. Example: group rename 3456 better_named_net"
const notEnoughArgsForUnjoin string = "Not enough arguments for: group unjoin. Provide group ID as first argument and device IPv6 fragment as the second. Example: group unjoin 3456 beef"
const notEnoughArgsForShowGroup string = "Not enough arguments for: group show. Provide numeric ID of the group. Example: group show 3456"
const notEnoughArgsForRenameDevice string = "Not enough arguments for: device rename. Provide an IPv6 (fc94...) and new name for the device. Example: device rename fc94:dead:beef:1234:9b8d:8beb:9999:8888 foodev"
