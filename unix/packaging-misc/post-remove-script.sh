#!/bin/bash

pidof -q systemd || false
if [ ! $? -ne 0 ]; then 
    systemctl daemon-reload
    systemctl stop husarnet
    systemctl disable husarnet
fi
