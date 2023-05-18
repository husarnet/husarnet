#!/bin/bash

command -v pidof >/dev/null || exit 0
command -v systemctl >/dev/null || exit 0

# Check whether system is *running* systemd
pidof -q systemd >/dev/null || exit 0

# Check whether husarnet unit file is properly installed and available
systemctl list-unit-files --all --quiet husarnet.service >/dev/null || exit 0

/usr/bin/husarnet daemon service-uninstall --silent
