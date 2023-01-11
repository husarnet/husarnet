#!/bin/bash
# Usage: ./integration-test.sh [JOIN_CODE] [Husarnet Dashboard User Name] [Husarnet Dashboard Password] [Husarnet Network Name]
set -euxo pipefail

join_code="${1}"
dashboard_login="${2}"
dashboard_pass="${3}"
network_name="${4}"

curl -s https://install.husarnet.com/install.sh | sudo bash

function CLEANUP {
   echo "Cleaning up"
   husarnet dashboard login "${dashboard_login}" "${dashboard_pass}"
   husarnet dashboard rm $(cat /etc/hostname) "${network_name}"
}

sudo husarnet-daemon &

husarnet daemon wait daemon
husarnet daemon wait base

sudo husarnet join "${join_code}"

husarnet daemon wait joined

managed_lines=$(cat /etc/hosts | grep -c '# managed by Husarnet')
if [ ${managed_lines} -lt 1 ]; then
   echo 'Husarnet failed saving hostnames to /etc/hosts file, cleaning up and exiting...'
   CLEANUP
   exit 1
else
   echo '/etc/hosts file is ok'
fi

echo 'SUCCESS: Basic integrity test went ok'
CLEANUP
