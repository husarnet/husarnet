#!/bin/bash
set -uo pipefail

catch() {
  if [ "$1" != "0" ]; then
    echo "$0, line $2: Error $1"
  fi
}
trap 'catch $? $LINENO' EXIT

# Rewrite old name to new one
if [ ! -z "${JOINCODE:-}" ]; then
    HUSARNET_JOIN_CODE="${JOINCODE}"
fi

# Check whether join code is set
if [ -z "${HUSARNET_JOIN_CODE:-}" ]; then
    echo "You have to set HUSARNET_JOIN_CODE environment variable in order for this container to work"
    exit 1
fi

# Start daemon
if [ -z "${HUSARNET_DEBUG:-}" ]; then
    HUSARNET_JOIN_CODE="${HUSARNET_JOIN_CODE}" husarnet daemon >/dev/null 2>&1
else
    HUSARNET_JOIN_CODE="${HUSARNET_JOIN_CODE}" husarnet daemon
fi
