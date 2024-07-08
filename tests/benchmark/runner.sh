#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

if [ $# -ne 1 ]; then
    echo "Usage: $0 <husarnet_version>"
    echo "Use 'dev' for **local** version"
    echo "Use 'latest' for **remote** version"
    exit 1
fi

HUSARNET_VERSION=$1

function husarnet_server () {
    HUSARNET_VERSION=${HUSARNET_VERSION} docker compose exec husarnet-server "${@}"
}
function husarnet_client () {
    HUSARNET_VERSION=${HUSARNET_VERSION} docker compose exec husarnet-client "${@}"
}
function iperf_client () {
    HUSARNET_VERSION=${HUSARNET_VERSION} docker compose exec iperf-client "${@}"
}

function compose() {
    HUSARNET_VERSION=${HUSARNET_VERSION} docker compose $@
}

pushd ${base_dir}/tests/benchmark

compose up --remove-orphans -d

echo "Compose up done, preparing images"

husarnet_server bash -c "apt-get update && apt-get install -y iputils-ping"
husarnet_client bash -c "apt-get update && apt-get install -y iputils-ping"

echo "Image preparation done, starting functional tests"

server_ip=$(husarnet_server bash -c "cat /var/lib/husarnet/id | cut -d ' ' -f 1")
client_ip=$(husarnet_client bash -c "cat /var/lib/husarnet/id | cut -d ' ' -f 1")

husarnet_server husarnet daemon whitelist add ${client_ip}
husarnet_client husarnet daemon whitelist add ${server_ip}

husarnet_server ping -c 1 ${client_ip}
husarnet_client ping -c 1 ${server_ip}

echo "Functional tests done, starting benchmark"

iperf_client iperf3 -c ${server_ip}

popd
