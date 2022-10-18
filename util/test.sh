#!/bin/bash
source $(dirname "$0")/bash-base.sh

docker compose -f builder/compose.yml up test
