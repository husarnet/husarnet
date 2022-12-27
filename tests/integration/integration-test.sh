#!/bin/bash
#Integration test #2
#Script is intended to be run from Superuser account.
#Before using this script set these values
#USERNAME - Your Husarnet dashboard username.
#PASSWORD - Your Husarnet dashboard password.
#NETWORK - Husarnet network name.
#JOIN_CODE - Husarnet join code.
CLEANUP=$(husarnet dashboard login ${USERNAME} ${PASSWORD} && husarnet dashboard rm $(cat /etc/hostname) ${NETWORK})
husarnet-daemon &
husarnet daemon wait daemon
husarnet daemon wait base
husarnet join ${JOIN_CODE} $(cat /etc/hostname)
TESTJOIN=$(husarnet status | grep -c 'Is joined?                 yes')
if [ $TESTJOIN -eq 0 ]; then
   echo 'Husarnet failed to join network'
   exit 1
else
   echo 'Network OK'
fi
TESTHOSTS=$(cat /etc/hosts | grep -c '# managed by Husarnet')
if [ $TESTHOSTS -eq 0 ]; then
   echo 'Husarnet failed saving hostnames to /etc/hosts file, cleaning up and exiting...'
   ${CLEANUP}
   exit 1
else
   echo '/etc/hosts file is ok'
fi
echo 'Basic integrity test went ok, cleaning up, and exitting...'
${CLEANUP}
exit 0