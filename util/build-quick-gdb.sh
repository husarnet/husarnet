#!/bin/bash
source $(dirname "$0")/bash-base.sh

${util_base}/build-quick.sh && sudo gdb -ex=run --args ${release_base}/husarnet-unix-amd64 daemon
