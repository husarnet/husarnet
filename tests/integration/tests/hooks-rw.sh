#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../../util/bash-base.sh

source $(dirname "$0")/../utils.sh

mkdir -p /var/lib/husarnet/hook.rw_request.d
mkdir -p /var/lib/husarnet/hook.rw_release.d

cat << EOF > /var/lib/husarnet/hook.rw_request.d/rw.sh
#!/bin/bash
set -euo pipefail

mount -o remount,rw /var/lib/husarnet

echo "INFO: rw_request hook triggered" >> /tmp/hooks/hooks.log
EOF

cat << EOF > /var/lib/husarnet/hook.rw_release.d/ro.sh
#!/bin/bash
set -euo pipefail

echo "INFO: rw_release hook triggered" >> /tmp/hooks/hooks.log

mount -o remount,ro /var/lib/husarnet
EOF

chmod +x /var/lib/husarnet/hook.rw_request.d/rw.sh
chmod +x /var/lib/husarnet/hook.rw_release.d/ro.sh

function abort {
    echo "ERROR: Aborting"
    ls -lR /tmp/hooks
    tail -n 100 /tmp/hooks/*.log
    cleanup
    exit 1
}

mkdir -p /tmp/hooks

cat << EOF > /var/lib/husarnet/config.json
{
    "user-settings": {
        "enableHooks": "true"
    }
}
EOF

mount -o remount,ro /var/lib/husarnet

start_daemon

join

sleep 5 # daemon needs this much to trigger ro hook
sleep 3 # add some for a good measure

grep "INFO: rw_request hook triggered" /tmp/hooks/hooks.log || abort
grep "INFO: rw_release hook triggered" /tmp/hooks/hooks.log || abort

husarnet status

echo "---"
tail -n 100 /tmp/hooks/*.log
echo "---"

mount -o remount,rw /var/lib/husarnet

cleanup
