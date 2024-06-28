#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

source $(dirname "$0")/../secrets-decrypted.sh

pushd ${base_dir}/platforms/esp32

pytest pytest_eth_husarnet.py --target esp32 --junitxml junit.xml --joincode ${esp32_join_code} --app-path ../../build

popd
