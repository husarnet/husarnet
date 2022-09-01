#!/bin/bash
source $(dirname "$0")/bash-base.sh

pushd ${build_tests_base}

./husarnet_tests

popd
