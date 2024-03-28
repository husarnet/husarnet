#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt

# This script is a wrapper around idf.py that sets the
# correct build and source dirs for our repo. It should
# be called instead of idf.py in the root of the repo.

idf.py -B build/esp32 -C platforms/esp32/ "$@"
