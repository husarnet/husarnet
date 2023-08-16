#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

if [ $# -gt 0 ]; then
    args="\"${*}\""
else
    args="bash"
fi

pushd ${base_dir}
docker run --rm -it --privileged --volume ${base_dir}:/app ghcr.io/husarnet/husarnet:builder -c ${args}
popd
