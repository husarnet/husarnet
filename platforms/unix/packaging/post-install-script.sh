#!/bin/bash

command -v pidof >/dev/null || exit 0
command -v systemctl >/dev/null || exit 0

# Check whether system is *running* systemd
pidof -q systemd >/dev/null || exit 0

systemctl daemon-reload

# Check whether husarnet unit file is properly installed and available
systemctl list-unit-files --all --quiet husarnet.service >/dev/null || exit 0

systemctl enable husarnet
systemctl restart husarnet
