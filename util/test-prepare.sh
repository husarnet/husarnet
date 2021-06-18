#!/bin/bash
source $(dirname "$0")/bash-base.sh

sudo apt update

# Install stuff
sudo apt install -y cppcheck libubsan1 python3 iproute2 iptables net-tools sqlite3
