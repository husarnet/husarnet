#!/bin/bash
# Copyright (c) 2022 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../util/bash-base.sh

echo "[HUSARNET BS] Building the builder"

pushd ${base_dir}/builder
DOCKER_BUILDKIT=0 docker build -t ghcr.io/husarnet/husarnet:builder .
popd

echo "To inspect the image manually run:"
echo './builder/shell.sh'
