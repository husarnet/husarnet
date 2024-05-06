#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

# This file is intended to be run only from inside Docker!

test_platform=${1}
test_file=${2}

case "${test_platform}" in
    docker)
        apt update
        
        # Test prerequisites
        apt install -y --no-install-recommends --no-install-suggests \
        jq curl iputils-ping ca-certificates gdb
    ;;
    
    ubuntu | debian)
        apt update
        
        # Install Husarnet deb
        apt install -y --no-install-recommends --no-install-suggests \
        /app/build/release/husarnet-linux-amd64.deb
        
        # Test prerequisites
        apt install -y --no-install-recommends --no-install-suggests \
        jq curl iputils-ping ca-certificates gdb
    ;;
    
    fedora)
        # Install Husarnet rpm
        yum install -y \
        /app/build/release/husarnet-linux-amd64.rpm
        
        # Test prerequisites
        yum install -y \
        jq curl iputils hostname ca-certificates gdb util-linux
        
    ;;
    
    *)
        echo "Unknown test platform ${test_platform}!"
        exit 4
    ;;
esac

${tests_base}/integration/tests/${test_file}.sh
exit $?
