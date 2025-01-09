#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
set -uo pipefail

catch() {
  if [ "$1" != "0" ]; then
    echo "$0, line $2: Error $1"
  fi
}
trap 'catch $? $LINENO' EXIT

if [ ! -z "${WAIT_HOSTNAMES:-}" ]; then
  export HUSARNET_WAIT_HOSTNAMES="${WAIT_HOSTNAMES}"
fi

if [ -z "${HUSARNET_WAIT_HOSTNAMES:-}" ]; then
  husarnet daemon wait daemon
  exit 0
fi

# Make sure no unecessary whitespaces are present (like newlines)
hostnames_array=($(echo ${HUSARNET_WAIT_HOSTNAMES} | tr , " "))
hostnames="${hostnames_array[@]}"

husarnet daemon wait hostnames ${hostnames}
