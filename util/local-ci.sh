#!/bin/bash
source $(dirname "$0")/bash-base.sh

${util_base}/update-workflows.sh
${util_base}/format.sh
${util_base}/build.sh
${util_base}/test.sh
${tests_base}/integration.sh

echo "[HUSARNET BS] Success!"
