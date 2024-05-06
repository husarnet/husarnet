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

arch=$1
build_type=$2

# Fail if arch is not amd64
if [ "$arch" != "amd64" ]; then
    echo "Unsupported architecture: $arch"
    exit 1
fi

# Fail if build type is not stable or nightly
if [ "$build_type" != "stable" ] && [ "$build_type" != "nightly" ]; then
    echo "Unsupported build type: $build_type"
    exit 1
fi

echo "[HUSARNET BS] Building Husarnet snap image for ${arch}"

cp ${platforms_base}/snap/snapcraft.yaml ${release_base}/snapcraft.yaml

pushd ${release_base}
snapcraft
popd

rm -f ${release_base}/snapcraft.yaml
