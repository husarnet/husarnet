#!/bin/bash
set -euo pipefail

input="/var/lib/husarnet/ip6tables_rules"
if test -f "$input"; then
  while IFS= read  line
  do
    while ip6tables --delete "$line" 2> /dev/null; do :; done

  done < "$input"
fi
rm -fr /var/lib/husarnet
