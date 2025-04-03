#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../../util/bash-base.sh

source $(dirname "$0")/../utils.sh

function setup_hook_simple {
    hook_name=$1

    mkdir -p /var/lib/husarnet/hook.${hook_name}.d
    cat << EOF > /var/lib/husarnet/hook.${hook_name}.d/01-test.sh
#!/bin/bash
set -euo pipefail

echo "INFO: ${hook_name} hook triggered" >> /tmp/hooks/hook.${hook_name}.d.log
curl --silent --include --max-time 1 --retry-connrefused http://127.0.0.1:16216/api/status >> /tmp/hooks/hook.${hook_name}.d.log
EOF

    chmod +x /var/lib/husarnet/hook.${hook_name}.d/01-test.sh
}

function abort {
    echo "ERROR: Aborting"
    ls -lR /tmp/hooks
    tail -n 100 /tmp/hooks/*.log
    cleanup
    exit 1
}

function verify_hook {
    hook_name=$1

    if [ ! -f /tmp/hooks/hook.${hook_name}.d.log ]; then
        echo "ERROR: ${hook_name} hook was not triggered at all"
        abort
    fi
    if [ $(cat /tmp/hooks/hook.${hook_name}.d.log | grep -c "INFO: ${hook_name} hook triggered") -lt 1 ]; then
        echo "ERROR: ${hook_name} hook was not triggered"
        abort
    fi
    if [ $(cat /tmp/hooks/hook.${hook_name}.d.log | grep -c "HTTP/1.1 200 OK") -lt 1 ]; then
        echo "ERROR: ${hook_name} hook did not return 200 OK"
        abort
    fi
}

mkdir -p /tmp/hooks

# setup_hook_simple "hosttable_changed" # We don't have any peers in that network at the moment
setup_hook_simple "whitelist_changed"
setup_hook_simple "joined"
# setup_hook_simple "reconnected" # We're not testing reconnections in this test
# setup_hook "rw_request" # RW hooks require special setup
# setup_hook "rw_release"

cat << EOF > /var/lib/husarnet/config.json
{
    "user-settings": {
        "enableHooks": "true"
    }
}
EOF

start_daemon

join

sleep 3 # Give some time to actually execute hooks

husarnet status

grep -c "DummyHooksManager" /tmp/husarnet-daemon.log && abort # This is a big fail indicating that hooks were not enabled immediately after startup

verify_hook "whitelist_changed"
verify_hook "joined"

cleanup
