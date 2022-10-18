#!/bin/bash
source $(dirname "$0")/bash-base.sh

${base_dir}/platforms/docker/build.sh amd64

docker compose -f builder/compose.yml up unix_amd64
# docker compose -f builder/compose.yml up unix_i386
# docker compose -f builder/compose.yml up unix_arm64
# docker compose -f builder/compose.yml up unix_armhf
# docker compose -f builder/compose.yml up unix_riscv64
docker compose -f builder/compose.yml up windows_win64
