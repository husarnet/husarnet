#!/bin/bash
# Copyright (c) 2022 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt

command -v pidof >/dev/null || exit 0
command -v systemctl >/dev/null || exit 0

# Check whether system is *running* systemd
pidof -q systemd >/dev/null || exit 0

# Check whether husarnet unit file is properly installed and available
systemctl list-unit-files --all --quiet husarnet.service >/dev/null || exit 0

/usr/bin/husarnet daemon service-uninstall --silent || exit 0
