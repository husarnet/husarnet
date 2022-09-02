#!/bin/bash
source $(dirname "$0")/bash-base.sh

sudo apt-get update

# Install stuff for cross building
sudo apt-get install -y clang-format
