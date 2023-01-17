#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

# WARNING this script is intended to be ran from inside Docker containers ONLY

echo "--- Preparations -----------------------------------------"

${tests_base}/integration/secrets-tool.sh decrypt

echo "--- Running tests ----------------------------------------"

${tests_base}/integration/tests/functional-basic.sh
${tests_base}/integration/tests/join-workflow.sh
