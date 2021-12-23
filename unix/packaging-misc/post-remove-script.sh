#!/bin/bash

pidof -q systemd || false
if [ ! $? -ne 0 ]; then 
    systemctl daemon-reload
    systemctl stop husarnet
    systemctl disable husarnet
fi

input="/var/lib/husarnet/ip6tables_rules"
if test -f "$input"; then
  while IFS= read line
  do
    while ip6tables --delete "$line" 2> /dev/null; do :; done

  done < "$input"
fi
