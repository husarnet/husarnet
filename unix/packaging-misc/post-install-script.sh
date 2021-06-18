#!/bin/bash
set -euo pipefail

# Check whether system is *running* systemd
pidof -q systemd || false
if [ ! $? -eq 0 ]; then 
    echo "No systemd running in the system. Will not install start scripts."
    exit 0
fi

systemctl daemon-reload
systemctl enable husarnet

echo "========================================"
echo "Husarnet installed/upgraded."
echo "Please restart it with:"
echo ""
echo "  systemctl restart husarnet"
echo ""
echo "========================================"
