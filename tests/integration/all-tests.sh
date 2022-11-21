#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

echo "--------------------------------------------"

${tests_base}/integration/functional-basic.sh
