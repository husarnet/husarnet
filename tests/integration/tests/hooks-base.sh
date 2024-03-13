#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../../util/bash-base.sh
source $(dirname "$0")/../utils.sh
stop_daemon
rm -rf /var/lib/husarnet/hook*/
start_daemon
sleep 8
join
husarnet status
husarnet daemon hooks enable
stop_daemon && start_daemon
sleep 8
join
if [husarnet status | grep 'Is joined?                 yes']
then
echo 'SUCCESS: Hooks base test went ok'
exit 0
else
echo 'FAIL: Hooks base test failed'
exit 1
fi