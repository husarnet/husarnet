#!/bin/bash
set -euo pipefail

input="/var/lib/husarnet/ip6tables_rules"
while IFS= read  line
do
  ip6tables --delete $line
done < "$input"

rm -fr /var/lib/husarnet
