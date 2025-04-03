#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../../util/bash-base.sh

source $(dirname "$0")/../utils.sh

start_daemon

api=$(curl staging.husarnet.com/license.json | jq -r '.api_servers[0]')
eb=$(curl staging.husarnet.com/license.json | jq -r '.eb_servers[0]')

ping -c 10 ${api}
ping -c 10 ${eb}

husarnet status
