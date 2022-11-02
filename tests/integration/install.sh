#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

apt update

apt install -y --no-install-recommends --no-install-suggests \
    /app/build/release/husarnet-unix-amd64.deb
