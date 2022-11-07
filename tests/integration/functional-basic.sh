#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

husarnet-daemon &

husarnet daemon wait daemon

husarnet status

husarnet daemon wait base

husarnet status

websetup=$(curl app.husarnet.com/license.json | jq -r '.websetup_host')

husarnet daemon whitelist add ${websetup}
ping -c 3 ${websetup}

husarnet status
