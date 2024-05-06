#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

# This file is intended to be run only from inside Docker!

if [ ${#} -ne 3 ]; then
    echo "Usage: runner.sh [test_platform] [test_file] [test_image]"
    exit 1
fi

test_platform=${1}
test_file=${2}
test_image=${3}

echo "=== Running integration test === ${test_file} === on ${test_platform} === on ${test_image} ==="

test_log_path=/tmp/test.log

function echo_fail {
    cat ${test_log_path}
    echo "                           "
    echo "                           "
    echo "  ______      _____ _      "
    echo " |  ____/\   |_   _| |     "
    echo " | |__ /  \    | | | |     "
    echo " |  __/ /\ \   | | | |     "
    echo " | | / ____ \ _| |_| |____ "
    echo " |_|/_/    \_\_____|______|"
    echo "                           "
    echo "                           "
    exit $1
}

function echo_success {
    echo "...completed successfully!"
}

(time ${tests_base}/integration/runner.sh $@ >>${test_log_path} 2>&1 && echo_success) || (echo_fail $?)
