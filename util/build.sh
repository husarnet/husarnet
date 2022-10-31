#!/bin/bash
source $(dirname "$0")/bash-base.sh

docker compose -f builder/compose.yml up --exit-code-from unix_amd64 unix_amd64
# docker compose -f builder/compose.yml up --exit-code-from unix_i386 unix_i386
# docker compose -f builder/compose.yml up --exit-code-from unix_arm64 unix_arm64
# docker compose -f builder/compose.yml up --exit-code-from unix_armhf unix_armhf
# docker compose -f builder/compose.yml up --exit-code-from unix_riscv64 unix_riscv64
docker compose -f builder/compose.yml up --exit-code-from windows_win64 windows_win64

${base_dir}/platforms/docker/build.sh amd64
