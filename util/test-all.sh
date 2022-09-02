#!/bin/bash
source $(dirname "$0")/bash-base.sh

${util_base}/test-cppcheck.sh
${util_base}/test-unit-build.sh
${util_base}/test-unit-run.sh
${util_base}/copyright-checker.py
