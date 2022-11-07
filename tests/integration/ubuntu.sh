#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

apt update

apt install -y --no-install-recommends --no-install-suggests \
    /app/build/release/husarnet-unix-amd64.deb

# Test prerequisites
apt install -y --no-install-recommends --no-install-suggests \
    jq curl iputils-ping

${tests_base}/integration/all-tests.sh
