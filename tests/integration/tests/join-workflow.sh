#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../../util/bash-base.sh

source $(dirname "$0")/../utils.sh

start_daemon

join

# This sleep is required to make /etc/hosts less flaky as we sometimes do rewrite or even delete it while changing the host table
sleep 10

echo "INFO: Testing /etc/hosts content"
managed_lines=$(cat /etc/hosts | grep -c '# managed by Husarnet')
if [ ${managed_lines} -lt 1 ]; then
   echo 'ERROR: Husarnet failed to save hostnames to /etc/hosts file, cleaning up and exiting...'
   cleanup
   exit 1
else
   echo 'INFO: /etc/hosts file seems ok'
fi

echo 'SUCCESS: Basic integrity test went ok'
cleanup
