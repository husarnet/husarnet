#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../util/bash-base.sh

if [ $# -gt 0 ]; then
    args="\"${*}\""
else
    args="bash"
fi

pushd ${base_dir}
docker run --rm -it --privileged --volume ${base_dir}:/app ghcr.io/husarnet/husarnet:builder -c ${args}
popd
