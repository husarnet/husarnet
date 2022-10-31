#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

${tests_base}/integration/install.sh
${tests_base}/integration/functional-basic.sh
