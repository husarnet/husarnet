#!/bin/bash
source $(dirname "$0")/bash-base.sh

docker_builder /app/daemon/format.sh
docker_builder /app/cli/format.sh
