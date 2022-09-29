#!/bin/bash
source $(dirname "$0")/bash-base.sh

pushd "${base_dir}"

${util_base}/build-cmake.sh amd64 unix
${util_base}/build-cli.sh amd64 unix
${util_base}/docker-prepare-paths.sh amd64

docker build -t husarnet:dev -f platforms/docker/Dockerfile .

echo "In order to start the container run:"
echo "docker run --privileged --rm -it --env "HUSARNET_JOIN_CODE=your-joincode" husarnet:dev"

popd
