#!/bin/bash
# Copyright (c) 2026 Husarion S.A.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../util/bash-base.sh

docker image push ghcr.io/husarnet/husarnet:builder
