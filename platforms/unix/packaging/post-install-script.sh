#!/bin/bash

command -v pidof >/dev/null || exit 0
command -v systemctl >/dev/null || exit 0

# Check whether system is *running* systemd
pidof -q systemd >/dev/null || exit 0

/usr/bin/husarnet daemon service-install
