#!/bin/bash

# TODO: if pre-remove works, this will be no-op, as it won't find husarnet.service in systemctl.
# Eventually the idea is to delete this file. I am leaving it for testing period though,
# as it should be harmless.

command -v pidof >/dev/null || exit 0
command -v systemctl >/dev/null || exit 0

# Check whether system is *running* systemd
pidof -q systemd >/dev/null || exit 0

# Check whether husarnet unit file is properly installed and available
systemctl list-unit-files --all --quiet husarnet.service >/dev/null || exit 0

echo "Disabling husarnet service in systemctl..."

systemctl stop husarnet
systemctl disable husarnet

systemctl daemon-reload
