#!/bin/bash
source $(dirname "$0")/bash-base.sh

docker compose -f builder/compose.yml run unix_amd64
# docker compose -f builder/compose.yml run unix_i386
# docker compose -f builder/compose.yml run unix_arm64
# docker compose -f builder/compose.yml run unix_armhf
# docker compose -f builder/compose.yml run unix_riscv64
docker compose -f builder/compose.yml run windows_win64

${base_dir}/platforms/docker/build.sh amd64
