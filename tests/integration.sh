#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

pushd ${base_dir}

docker compose -f builder/compose.yml up --exit-code-from test_ubuntu_18_04 test_ubuntu_18_04
docker compose -f builder/compose.yml up --exit-code-from test_ubuntu_20_04 test_ubuntu_20_04
docker compose -f builder/compose.yml up --exit-code-from test_ubuntu_22_04 test_ubuntu_22_04

popd
