#!/bin/bash
source $(dirname "$0")/../../../util/bash-base.sh

yum install -y \
    /app/build/release/husarnet-unix-amd64.rpm

# Test prerequisites
yum install -y \
    jq curl iputils

${tests_base}/integration/all-tests.sh
