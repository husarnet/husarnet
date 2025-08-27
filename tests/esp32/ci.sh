#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

source $(dirname "$0")/../secrets-decrypted.sh

runner_hostname=$(hostname -f)
esp32_hostname="esp32-integration-test"
network_name="integration_tests_esp"

# Initialize ESP-IDF environment
if ! command -v idf.py &> /dev/null; then
  echo "ESP-IDF environment is not initialized, initializing..."
  . /esp/esp-idf/export.sh
fi

# Install iperf2
if ! command -v iperf &> /dev/null; then
  sudo apt install -y --no-install-recommends --no-install-suggests iperf
fi

# Install husarnet daemon
if ! command -v husarnet &> /dev/null; then
  sudo apt install -y --no-install-recommends --no-install-suggests \
  ${base_dir}/build/release/husarnet-linux-amd64.deb

  # Start husarnet daemon
  sudo husarnet-daemon &> /tmp/husarnet-daemon.log &
  
  # Join to Husarnet network
  echo "INFO: Waiting for deamon connectivity"
  husarnet daemon wait daemon
  echo "INFO: Waiting for Base Server connectivity"
  husarnet daemon wait base
  sudo husarnet join ${esp32_join_code} ${runner_hostname}
else
  # Those are reduntant but we want to test as many items as possible
  echo "INFO: Waiting for deamon connectivity"
  husarnet daemon wait daemon
  echo "INFO: Waiting for Base Server connectivity"
  husarnet daemon wait base
fi

pushd ${base_dir}/platforms/esp32

# Run tests
pytest pytest_eth_husarnet.py --target esp32 --junitxml junit.xml --joincode ${esp32_join_code} --app-path ../../build

# Cleanup
echo "INFO: Cleaning up"

sudo pkill husarnet-daemon
echo "WARNING: This 'killed' message is a good thing! We're calling it on purpose"

# TODO: align to the new dashboard api
# husarnet dashboard login "${dashboard_login}" "${dashboard_pass}"
# husarnet dashboard rm ${runner_hostname} "${network_name}"
# husarnet dashboard rm ${esp32_hostname} "${network_name}"
# husarnet dashboard device rm "${runner_hostname}"
# husarnet dashboard device rm "${esp32_hostname}"

echo "INFO: Cleaned up successfully"

popd
