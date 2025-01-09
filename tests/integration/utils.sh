#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../../util/bash-base.sh

source $(dirname "$0")/../../secrets-decrypted.sh

my_hostname=$(hostname -f)

function cleanup {
    echo "INFO: Cleaning up"

    stop_daemon

    husarnet dashboard login "${dashboard_login}" "${dashboard_pass}"
    husarnet dashboard rm "${my_hostname}" "${network_name}"
    husarnet dashboard device rm "${my_hostname}"

    echo "INFO: Cleaned up successfully"
}

function start_daemon {
    gdb -batch -ex "set env HUSARNET_DASHBOARD_FQDN = staging.husarnet.com" -ex "catch throw" -ex "run" -ex "bt full" -ex "quit" --args husarnet-daemon 2>&1 | tee /tmp/husarnet-daemon.log &

    # Those are redundant but we want to test as many items as possible
    echo "INFO: Waiting for deamon connectivity"
    husarnet daemon wait daemon
    echo "INFO: Waiting for Base Server connectivity"
    husarnet daemon wait base

    husarnet status
}

function stop_daemon {
    pkill husarnet-daemon
    echo "WARNING: This 'killed' message is a good thing! We're calling it on purpose"
}

function join {
    echo "INFO: Joining"
    husarnet join "${join_code}"

    # This is kinda redundant too
    echo "INFO: Waiting until joined"
    husarnet daemon wait joined
}
