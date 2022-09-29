#!/bin/bash
set -uo pipefail

catch() {
  if [ "$1" != "0" ]; then
    echo "$0, line $2: Error $1"
  fi
}
trap 'catch $? $LINENO' EXIT

# Rewrite old names to new ones
if [ ! -z "${JOINCODE:-}" ]; then
    export HUSARNET_JOIN_CODE="${JOINCODE}"
fi
if [ ! -z "${HUSARNET_JOINCODE:-}" ]; then
    export HUSARNET_JOIN_CODE="${HUSARNET_JOINCODE}"
fi
if [ ! -z "${HOSTNAME:-}" ]; then
    export HUSARNET_HOSTNAME="${HOSTNAME}"
fi

# Start daemon
if [ -z "${HUSARNET_DEBUG:-}" ]; then
    husarnet-daemon >/dev/null 2>&1
else
    husarnet-daemon
fi
