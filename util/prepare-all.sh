#!/bin/bash
source $(dirname "$0")/bash-base.sh

${util_base}/build-prepare.sh
${util_base}/test-prepare.sh
${util_base}/deploy-prepare.sh
