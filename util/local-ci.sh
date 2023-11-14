#!/bin/bash
# Copyright (c) 2022 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/bash-base.sh

${util_base}/update-workflows.sh
${util_base}/format.sh
${util_base}/build.sh
${util_base}/test.sh
${tests_base}/integration.sh

echo "[HUSARNET BS] Success!"
