#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
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
if [ "${HOSTNAME:-}" != $(hostname) ]; then
    export HUSARNET_HOSTNAME="${HOSTNAME}"
fi

# Start daemon
if [ -z "${HUSARNET_DEBUG:-}" ]; then
    # 0 - ERROR(1) and CRITICAL(0)
    HUSARNET_LOG_VERBOSITY=1 husarnet-daemon
else
    # Basically everything including DEBUG(4) (for nightly builds, stable ones are capped at INFO(3))
    HUSARNET_LOG_VERBOSITY=100 husarnet-daemon
fi
