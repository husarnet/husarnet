#!/bin/bash
source $(dirname "$0")/bash-base.sh

docker_builder /app/platforms/linux/build.sh amd64 nightly
# docker_builder /app/platforms/linux/build.sh armhf nightly
# docker_builder /app/platforms/linux/build.sh arm64 nightly
docker_builder /app/platforms/windows/build.sh
# docker_builder /app/platforms/esp32/build.sh
docker_builder /app/platforms/macos/build.sh arm64

${base_dir}/platforms/docker/build.sh amd64
