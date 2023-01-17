#!/bin/bash
source $(dirname "$0")/bash-base.sh

docker_builder /app/platforms/unix/build.sh amd64
docker_builder /app/platforms/windows/build.sh

${base_dir}/platforms/docker/build.sh amd64
