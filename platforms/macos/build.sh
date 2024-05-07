#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

# This file is intended to be run only from inside Docker!

if [ ! "$#" -eq 2 ]; then
    echo "Usage: $0 <architecture> [stable/nightly]"
    exit 1
fi

platform=macos
arch="$1"
build_type=$2

platform_base=${base_dir}/platforms/${platform}

echo "[HUSARNET BS] Building Husarnet for MacOS/${arch}"

${base_dir}/daemon/build.sh ${platform} ${platform}_${arch} ${build_type}
${base_dir}/cli/build.sh ${platform} ${arch}

pushd ${release_base}
tar -zcf husarnet-macos-${arch}.tgz husarnet-macos-${arch} husarnet-daemon-macos-macos_${arch}
popd
