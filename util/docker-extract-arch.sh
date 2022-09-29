#!/bin/bash
source $(dirname "$0")/bash-base.sh

if [ "$#" -eq 0 ] || [ "$#" -gt 1 ]; then
    echo "Usage: $0 <docker architecture>"
    exit 1
fi

if [ "$1" = "linux/amd64" ]; then
    echo "amd64"
    exit 0
elif [ "$1" = "linux/arm/v7" ]; then
    echo "armhf"
    exit 0
elif [ "$1" = "linux/arm64" ] || [ "$1" = "linux/arm64/v8" ]; then
    echo "arm64"
    exit 0
fi

exit 1
