#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

if [ ! "$#" -eq 1 ]; then
    echo "Usage: $0 <architecture>"
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

base_platform=unix

if [ ${in_ci} == false ]; then
    pushd ${base_dir}
    docker compose -f builder/compose.yml up unix_amd64
    popd
fi

cp ${release_base}/husarnet-daemon-${base_platform}-${arch} ${release_base}/husarnet-daemon
cp ${release_base}/husarnet-${base_platform}-${arch} ${release_base}/husarnet

if [ ${in_ci} == false ]; then
    pushd ${base_dir}

    docker build -t husarnet:dev -f platforms/docker/Dockerfile .

    echo "In order to start the container run:"
    echo "docker run --privileged --rm -it --env "HUSARNET_JOIN_CODE=your-joincode" husarnet:dev"

    popd
fi
