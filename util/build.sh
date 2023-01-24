#!/bin/bash
source $(dirname "$0")/bash-base.sh

docker_builder /app/platforms/unix/build.sh amd64
docker_builder /app/platforms/windows/build.sh
# docker_builder /app/platforms/esp32/build.sh
# docker_builder /app/platforms/macos/build.sh arm64

${base_dir}/platforms/docker/build.sh amd64
