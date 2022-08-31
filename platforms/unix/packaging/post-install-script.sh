#!/bin/bash
# Check whether system is *running* systemd
pidof -q systemd || false
if [ ! $? -ne 0 ]; then 
    systemctl daemon-reload
    systemctl enable husarnet
    systemctl restart husarnet
fi
