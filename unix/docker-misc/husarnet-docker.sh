#!/bin/bash
set -euo pipefail

catch() {
  if [ "$1" != "0" ]; then
    echo "$0, line $2: Error $1"
  fi
}
trap 'catch $? $LINENO' EXIT

# If ${JOINCODE} is empty - quit
if [ -z "${JOINCODE:-}" ]; then
    echo "You have to set JOINCODE environment variable in order for this container to work"
    exit 1
fi

# Start daemon
if [ -z "${HUSARNET_DEBUG:-}" ]; then
    husarnet daemon >/dev/null 2>&1 &
else
    husarnet daemon &
fi

daemon_pid=$!

# Wait until the daemon is ready
while true; do
    if [ $(husarnet status 2>&1 | grep "control socket" | wc -l) -eq 0 ]; then
        break
    elif [ ! -e /proc/${daemon_pid} ]; then
        echo "Daemon has quit unexpectedly!"
        exit 2
    else
        echo "Waiting for the husarnet daemon to start"
        sleep 1
    fi
done

# Join the network
husarnet join ${JOINCODE} ${HOSTNAME}

# Print status
husarnet status | grep "Husarnet IP address"

# Keep this process running until daemon quits for any reason
wait ${daemon_pid}
