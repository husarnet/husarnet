#!/bin/bash
source $(dirname "$0")/bash-base.sh

docker_builder /app/platforms/linux/build.sh amd64 nightly
# docker_builder /app/platforms/linux/build.sh armhf nightly
# docker_builder /app/platforms/linux/build.sh arm64 nightly
docker_builder /app/platforms/windows/build.sh nightly
# docker_builder /app/platforms/esp32/build.sh nightly
# We are currently NOT cross-compiling for macOS. Reenable when resolved.
# docker_builder /app/platforms/macos/build.sh arm64 nightly

${base_dir}/platforms/docker/build.sh amd64 nightly
