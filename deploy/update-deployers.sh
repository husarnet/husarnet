#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

echo "[HUSARNET BS] Building the deployment suite for pkg packages"

pushd ${base_dir}/deploy/pkg
DOCKER_BUILDKIT=0 docker build -t ghcr.io/husarnet/husarnet:deploy-pkg .
popd

if [[ $# -eq 1 && "$1" == "push" ]]; then
    docker image push ghcr.io/husarnet/husarnet:deploy-pkg
fi
