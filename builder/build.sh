#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

echo "[HUSARNET BS] Building the builder"

pushd ${base_dir}/builder
DOCKER_BUILDKIT=0 docker build -t ghcr.io/husarnet/husarnet:builder .
popd

echo "To inspect the image manually run:"
echo './builder/shell.sh'
