#!/bin/bash
source $(dirname "$0")/bash-base.sh

pushd "${base_dir}"

docker build -t husarnet:dev -f unix/docker/Dockerfile --build-arg TARGETPLATFORM="amd64 unix" .

echo "In order to start the container run:"
echo "docker run --privileged --rm -it --env "HUSARNET_JOIN_CODE=your-joincode" husarnet:dev"

popd
