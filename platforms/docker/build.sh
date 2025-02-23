#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

# This file is intended **not** to be run from inside Docker!

if [ ! "$#" -eq 2 ]; then
    echo "Usage: $0 <architecture> [stable/nightly]"
    exit 1
fi

arch=$1
build_type=$2
base_platform=linux

pushd ${base_dir}

# Technically this script does not support multiple architectures because it does not use buildx directly. All the provisionings here are just a preparation for such support

echo "[HUSARNET BS] Building docker image for ${arch}"

echo "[HUSARNET BS] NOT building the binaries itself. Use './util/build.sh docker' for full workflow"

docker build --build-arg HUSARNET_ARCH=${arch} --tag husarnet:dev --tag husarnet/husarnet-nightly:dev --file platforms/docker/Dockerfile .

echo "In order to start the container run:"
echo 'docker run --privileged --rm -it --env "HUSARNET_JOIN_CODE=your-joincode" husarnet/husarnet-nightly:dev'

popd
