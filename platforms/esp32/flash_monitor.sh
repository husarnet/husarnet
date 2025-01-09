#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt

set -e

source $(dirname "$0")/../../util/bash-base.sh

if [ $# -ne 1 ]; then
  echo "Usage: $0 <serial_port>"
  exit 1
fi

platforms/esp32/idf.py -p "$1" -b 921600 flash
tio -b 115200 "$1"
