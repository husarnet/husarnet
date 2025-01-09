#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/bash-base.sh

preset=${1:-all}

# all - do all possible actions (default) - run this before commit!
# format - only format
# unit - only run unit tests
# gh - update github actions workflows
# build - build all platforms
# linux - build linux only
# docker - build linux and pack it into docker
# integration - build linux, docker and run integration tests

if [ "$preset" == "all" ] || [ "$preset" == "builder" ]; then
    ${base_dir}/builder/build.sh
fi

if [ "$preset" == "all" ] || [ "$preset" == "format" ]; then
    ${util_base}/format.sh
fi

if [ "$preset" == "all" ] || [ "$preset" == "unit" ]; then
    ${util_base}/test.sh
fi

if [ "$preset" == "all" ] || [ "$preset" == "build" ]; then
    ${util_base}/build.sh
elif [ "$preset" == "linux" ]; then
    ${util_base}/build.sh linux
elif [ "$preset" == "docker" ] || [ "$preset" == "integration" ]; then
    ${util_base}/build.sh docker
fi

if [ "$preset" == "all" ] || [ "$preset" == "integration" ]; then
    ${tests_base}/integration.sh
fi

if [ "$preset" == "all" ] || [ "$preset" == "gh" ]; then
    ${util_base}/update-workflows.sh
fi

echo "[HUSARNET BS] Success!"
