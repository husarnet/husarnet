my_hostname=$(hostname -f)

function CLEANUP {
   echo "INFO: Cleaning up"

   pkill husarnet-daemon
   echo "This 'killed' message is a good thing! We need to stop husarnet-daemon before we remove it from the dashboard"

   husarnet dashboard login "${dashboard_login}" "${dashboard_pass}"
   husarnet dashboard rm "${my_hostname}" "${network_name}"
   husarnet dashboard device rm "${my_hostname}"

   echo "INFO: Cleaned up successfully"
}

husarnet-daemon &

# Those are reduntant but we want to test as many items as possible
husarnet daemon wait daemon
husarnet daemon wait base

husarnet join "${join_code}"

# This is kinda redundant too
husarnet daemon wait joined

managed_lines=$(cat /etc/hosts | grep -c '# managed by Husarnet')
if [ ${managed_lines} -lt 1 ]; then
   echo 'ERROR: Husarnet failed to save hostnames to /etc/hosts file, cleaning up and exiting...'
   CLEANUP
   exit 1
else
   echo 'INFO: /etc/hosts file seems ok'
fi

echo 'SUCCESS: Basic integrity test went ok'
CLEANUP
