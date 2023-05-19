// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/linux/privileged_process.h"

#include <initializer_list>
#include <map>
#include <string>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/socket.h>

#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/shared_unix_windows/filesystem.h"
#include "husarnet/ports/shared_unix_windows/hosts_file_manipulation.h"

#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "privileged_process.h"

const static std::string hostnamePath = "/etc/hostname";
const static std::string configDir = "/var/lib/husarnet/";

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
    LOG_ERROR("cannot update hostname to %s", newHostname.c_str());
    return false;
  }

  return true;
}

json PrivilegedProcess::handleNotifyReady(json data)
{
  Port::notifyReady();
  return true;
}

json PrivilegedProcess::handleRunHook(json data)
{
  auto path = data.get<std::string>();
  Port::runScripts(configDir + path);
  return true;
}

json PrivilegedProcess::handleCheckHookExists(json data)
{
  auto path = data.get<std::string>();
  return Port::checkScriptsExist(configDir + path);
}

json PrivilegedProcess::handleResolveToIp(json data)
{
  return Port::resolveToIp(data.get<std::string>()).toString();
}

void PrivilegedProcess::run()
{
  // so parent can't ptrace us
  if(prctl(PR_SET_DUMPABLE, 0) < 0) {
    perror("PR_SET_DUMPABLE");
    exit(1);
  }
  prctl(PR_SET_PDEATHSIG, SIGKILL);

  char recvBuffer[40000];

  while(true) {
    long s = recv(fd, recvBuffer, sizeof(recvBuffer), 0);
    if(s < 0) {
      exit(2);
    }

    auto request = json::parse(recvBuffer);
    PrivilegedMethod endpoint = *PrivilegedMethod::_from_string_nothrow(
        request["endpoint"].get<std::string>().c_str());
    json data = request["data"];

    json response;

    switch(endpoint) {
      case +PrivilegedMethod::updateHostsFile:
        response = handleUpdateHostsFile(data);
        break;
      case +PrivilegedMethod::getSelfHostname:
        response = handleGetSelfHostname(data);
        break;
      case +PrivilegedMethod::setSelfHostname:
        response = handleSetSelfHostname(data);
        break;
      case +PrivilegedMethod::notifyReady:
        response = handleNotifyReady(data);
        break;
      case +PrivilegedMethod::runHook:
        response = handleRunHook(data);
        break;
      case +PrivilegedMethod::checkHook:
        response = handleCheckHookExists(data);
        break;
      case +PrivilegedMethod::resolveToIp:
        response = handleResolveToIp(data);
        break;
    }

    std::string txBuffer = response.dump(0);

    if(send(fd, txBuffer.data(), txBuffer.size() + 1, 0) < 0) {
      perror("send");
      exit(1);
    }
  }
}
