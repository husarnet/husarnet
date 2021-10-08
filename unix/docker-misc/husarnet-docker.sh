#!/bin/bash
set -uo pipefail

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

# Waiting until Husarnet daemon is ready
echo "[step 1/3] Waiting for Husarnet daemon to start"

while true; do
    if [ $(husarnet status 2>&1 | grep "request failed" | wc -l) -eq 0 ]; then
        break
    elif [ ! -e /proc/${daemon_pid} ]; then
        echo "Daemon has quit unexpectedly!"
        exit 2
    else
        echo "..."
        sleep 1
    fi
done

echo "done"
echo ""

# Waiting until Base Server connection is established
echo "[step 2/3] Waiting for Base Server connection"

while [ $(husarnet status | grep "ERROR" | wc -l) -gt 0 ]; 
do 
    echo "..."
    sleep 1
done

echo "done"
echo ""

# Joining the network
echo "[step 3/3] Joining to Husarnet network"

husarnet join ${JOINCODE} ${HOSTNAME}

# Print status
husarnet status | grep "Husarnet IP address"

# Keep this process running until daemon quits for any reason
wait ${daemon_pid}
