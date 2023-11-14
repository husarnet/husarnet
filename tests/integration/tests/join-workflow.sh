#!/bin/bash
# Copyright (c) 2022 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../../util/bash-base.sh

source $(dirname "$0")/../secrets-decrypted.sh

my_hostname=$(hostname -f)

function CLEANUP {
   echo "INFO: Cleaning up"

   pkill husarnet-daemon
   echo "WARNING: This 'killed' message is a good thing! We need to stop husarnet-daemon before we remove it from the dashboard"

   husarnet dashboard login "${dashboard_login}" "${dashboard_pass}"
   husarnet dashboard rm "${my_hostname}" "${network_name}"
   husarnet dashboard device rm "${my_hostname}"

   echo "INFO: Cleaned up successfully"
}

gdb -batch -ex "catch throw" -ex "run" -ex "bt full" -ex "quit" --args husarnet-daemon &

# Those are reduntant but we want to test as many items as possible
echo "INFO: Waiting for deamon connectivity"
husarnet daemon wait daemon
echo "INFO: Waiting for Base Server connectivity"
husarnet daemon wait base

echo "INFO: Joining"
husarnet join "${join_code}"

# This is kinda redundant too
echo "INFO: Waiting until joined"
husarnet daemon wait joined

# This sleep is required to make /etc/hosts less flaky as we sometimes do rewrite or even delete it while changing the host table
sleep 10

echo "INFO: Testing /etc/hosts content"
managed_lines=$(cat /etc/hosts | grep -c '# managed by Husarnet')
if [ ${managed_lines} -lt 1 ]; then
   echo 'ERROR: Husarnet failed to save hostnames to /etc/hosts file, cleaning up and exiting...'
   CLEANUP
   exit 1
else
   echo 'INFO: /etc/hosts file seems ok'
fi

echo 'SUCCESS: Basic integrity test went ok'
CLEANUP
