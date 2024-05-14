#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

# This file is intended **not** to be run from inside Docker!

if [ ! "$#" -eq 2 ]; then
    echo "Usage: $0 <architecture> [stable/nightly]"
    exit 1
fi

if [ "$1" = "linux/amd64" ]; then
    arch="amd64"
elif [ "$1" = "linux/arm/v7" ]; then
    arch="armhf"
elif [ "$1" = "linux/arm64" ] || [ "$1" = "linux/arm64/v8" ]; then
    arch="arm64"
else
    echo "[HUSARNET BS] Unknown docker arch $1"
    arch=$1
fi

build_type=$2
base_platform=linux

echo "[HUSARNET BS] Building docker image for ${arch}"

if [ ${in_ci} == false ]; then
    pushd ${base_dir}
    docker_builder /app/platforms/linux/build.sh amd64 $build_type
    popd
fi

cp ${release_base}/husarnet-${base_platform}-${arch}.deb ${release_base}/husarnet-${base_platform}.deb

# GH artifacts do not have proper flags
chmod +x ${release_base}/husarnet-${base_platform}.deb

if [ ${in_ci} == false ]; then
    pushd ${base_dir}

    docker build -t husarnet:dev -f platforms/docker/Dockerfile .

    echo "In order to start the container run:"
    echo "docker run --privileged --rm -it --env "HUSARNET_JOIN_CODE=your-joincode" husarnet:dev"

    popd
fi
