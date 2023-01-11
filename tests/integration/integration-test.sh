#!/bin/bash
#Integration test #2
#Script is intended to be run from Superuser account.
#Usage: ./integration-test.sh [JOIN_CODE] [Husarnet Dashboard User Name] [Husarnet Dashboard Password] [Husarnet Network Name]
curl -s https://install.husarnet.com/install.sh | sudo bash
function CLEANUP {
husarnet dashboard login "${2}" "${3}" && husarnet dashboard rm $(cat /etc/hostname) "${4}"
}
sudo husarnet-daemon &
husarnet daemon wait daemon
husarnet daemon wait base
husarnet join ${1} $(cat /etc/hostname)
husarnet status | grep 'Is joined?' | grep 'yes'
if [ ${?} -ne 0 ]; then
   echo 'Husarnet failed to join network'
   exit 1
else
   echo 'Network OK'
fi
cat /etc/hosts | grep -c '# managed by Husarnet'
if [ ${?} -ne 0 ]; then
   echo 'Husarnet failed saving hostnames to /etc/hosts file, cleaning up and exiting...'
   CLEANUP
   exit 1
else
   echo '/etc/hosts file is ok'
fi
echo 'Basic integrity test went ok, cleaning up, and exitting...'
CLEANUP
exit 0
