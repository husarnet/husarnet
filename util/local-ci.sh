#!/bin/bash
source $(dirname "$0")/bash-base.sh

${util_base}/format.sh
${util_base}/test.sh
${util_base}/build.sh

echo "[HUSARNET BS] Success!"
