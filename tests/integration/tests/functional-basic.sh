#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../../util/bash-base.sh

source $(dirname "$0")/../utils.sh

start_daemon

websetup=$(curl app.husarnet.com/license.json | jq -r '.websetup_host')

husarnet daemon whitelist add ${websetup}

ping -c 10 ${websetup}

husarnet status
