#!/bin/bash
source $(dirname "$0")/bash-base.sh

# ${util_base}/build-quick.sh
${util_base}/build-cli.sh

sudo --preserve-env="HUSARNET_DASHBOARD_FQDN" ${build_base}/bin/husarnet $@
