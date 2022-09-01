#!/bin/bash
source $(dirname "$0")/bash-base.sh

${util_base}/build-cli.sh amd64 unix
${build_base}/release/husarnet-unix-amd64 $@
