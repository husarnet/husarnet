#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

source $(dirname "$0")/../secrets-decrypted.sh

# Initialize ESP-IDF environment
if ! command -v idf.py &> /dev/null; then
  echo "ESP-IDF environment is not initialized, initializing..."
  . /esp/esp-idf/export.sh
fi

# Start husarnet daemon
gdb -batch -ex "catch throw" -ex "run" -ex "bt full" -ex "quit" --args husarnet-daemon 2>&1 | tee /tmp/husarnet-daemon.log &

# Those are reduntant but we want to test as many items as possible
echo "INFO: Waiting for deamon connectivity"
husarnet daemon wait daemon
echo "INFO: Waiting for Base Server connectivity"
husarnet daemon wait base

pushd ${base_dir}/platforms/esp32

# Run tests
pytest pytest_eth_husarnet.py --target esp32 --junitxml junit.xml --joincode ${esp32_join_code} --app-path ../../build

popd
