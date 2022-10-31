#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

apt update
apt install -y /app/build/release/husarnet-unix-amd64.deb

husarnet-daemon &

husarnet daemon wait daemon

husarnet status

husarnet daemon wait base

husarnet status
