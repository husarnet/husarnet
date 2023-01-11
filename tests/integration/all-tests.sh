#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

# WARNING this script is intended to be ran from inside Docker containers ONLY

echo "--------------------------------------------"

${tests_base}/integration/tests/functional-basic.sh
