#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/bash-base.sh

echo "[HUSARNET BS] Updating GitHub workflows"

pushd ${base_dir}
docker_builder /app/.github/update-workflows.sh
popd

echo "To inspect the image manually run:"
echo './builder/run.sh'
