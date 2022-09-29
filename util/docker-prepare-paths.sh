#!/bin/bash
source $(dirname "$0")/bash-base.sh

if [ ! "$#" -eq 1 ]; then
    echo "Usage: $0 <arch>"
    exit 1
fi

arch=$1

cp ${release_base}/husarnet-daemon-unix-${arch} ${release_base}/husarnet-daemon
cp ${release_base}/husarnet-unix-${arch} ${release_base}/husarnet
