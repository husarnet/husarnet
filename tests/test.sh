#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

${tests_base}/copyright-checker.py
${tests_base}/cppcheck.sh
${tests_base}/unit.sh
