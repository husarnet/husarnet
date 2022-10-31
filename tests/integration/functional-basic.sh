#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

husarnet-daemon &

husarnet daemon wait daemon

husarnet status

husarnet daemon wait base

husarnet status
