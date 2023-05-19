// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/macos/privileged_process.h"

#include <initializer_list>
#include <map>
#include <string>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/shared_unix_windows/filesystem.h"
#include "husarnet/ports/shared_unix_windows/hosts_file_manipulation.h"

#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

const static std::string hostnamePath = "/etc/hostname";

PrivilegedProcess::PrivilegedProcess()
{
}

void PrivilegedProcess::init(int fd)
{
  this->fd = fd;
}

json PrivilegedProcess::handleUpdateHostsFile(json data)
{
  std::map<std::string, IpAddress> dataMap;

  for(auto [hostname, address] : std::map<std::string, std::string>(data)) {
    dataMap[hostname] = IpAddress::parse(address);
  }

  return updateHostsFileInternal(dataMap);
}

json PrivilegedProcess::handleGetSelfHostname(json data)
{
  return rtrim(Port::readFile(hostnamePath));
}

json PrivilegedProcess::handleSetSelfHostname(json data)
{
  auto newHostname = data.get<std::string>();

  if(!validateHostname(newHostname)) {
    return false;
  }

  if(newHostname == handleGetSelfHostname({})) {
    return true;
  }

  Port::writeFile(hostnamePath, newHostname);

  if(system("hostname -F /etc/hostname") != 0) {
    LOG_WARNING("cannot update hostname to %s", newHostname.c_str());
    return false;
  }

  return true;
}

json PrivilegedProcess::handleNotifyReady(json data)
{
  Port::notifyReady();
  return true;
}

json PrivilegedProcess::handleResolveToIp(json data)
{
  return Port::resolveToIp(data.get<std::string>()).toString();
}

void PrivilegedProcess::run()
{
}
