#!/bin/bash
source $(dirname "$0")/bash-base.sh

docker_builder /app/tests/test.sh
