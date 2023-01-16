#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

# This file is intended to be run only from inside Docker!

if [ ${#} -ne 2 ]; then
    echo "Usage: runner.sh [test_platform] [test_file]"
    exit 1
fi

test_platform=${1}
test_file=${2}

echo "=== Running integration test === ${test_file} === on ${test_platform} ==="

source $(dirname "$0")/common.sh

source ${tests_base}/integration/platforms/${test_platform}.sh

${tests_base}/integration/secrets-tool.sh decrypt
source $(dirname "$0")/secrets-decrypted.sh

source ${tests_base}/integration/tests/${test_file}.sh
