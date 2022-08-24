#!/bin/bash
source $(dirname "$0")/bash-base.sh

# ${util_base}/build-quick.sh
${util_base}/build-cli.sh amd64 unix

# sudo --preserve-env="HUSARNET_DASHBOARD_FQDN" ${build_base}/bin/husarnet-linux-amd64 $@
${build_base}/bin/husarnet-linux-amd64 $@
