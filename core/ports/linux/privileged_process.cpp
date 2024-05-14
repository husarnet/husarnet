// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/linux/privileged_process.h"

#include <initializer_list>
#include <map>
#include <string>
#include <thread>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "husarnet/ports/port_interface.h"
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

bool PrivilegedProcess::hasHostnameFile()
{
  return Port::isFile(hostnamePath);
}

json PrivilegedProcess::handleGetSelfHostname(json data)
{
  if(hasHostnameFile())
    return rtrim(Port::readFile(hostnamePath));

  // On some platforms (i.e. OpenWRT) the hostname file does not exist
  LOG_WARNING(
      "hostname file does not exist on this system, derriving one from the "
      "host id");

  // Use unique hostid as a hostname
  long hostid = gethostid();
  std::vector<unsigned char> hostid_bytes = {
      (unsigned char)(hostid >> 24),
      (unsigned char)(hostid >> 16),
      (unsigned char)(hostid >> 8),
      (unsigned char)(hostid >> 0),
  };

  std::string hostid_str = encodeHex(hostid_bytes);

  return "device-" + hostid_str;
}

json PrivilegedProcess::handleSetSelfHostname(json data)
{
  auto newHostname = data.get<std::string>();

  if(!hasHostnameFile()) {
    LOG_ERROR(
        "cannot set hostname, hostname file does not exist on this system");
    return false;
  }

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

    Port::startThread(
        [=]() {
          auto request = json::parse(recvBuffer);
          PrivilegedMethod endpoint = *PrivilegedMethod::_from_string_nothrow(
              request["endpoint"].get<std::string>().c_str());
          int query_id = request["query_id"].get<int>();
          json data = request["data"];

          json response = {};

          switch(endpoint) {
            case +PrivilegedMethod::updateHostsFile:
              response["data"] = handleUpdateHostsFile(data);
              break;
            case +PrivilegedMethod::getSelfHostname:
              response["data"] = handleGetSelfHostname(data);
              break;
            case +PrivilegedMethod::setSelfHostname:
              response["data"] = handleSetSelfHostname(data);
              break;
            case +PrivilegedMethod::notifyReady:
              response["data"] = handleNotifyReady(data);
              break;
            case +PrivilegedMethod::runHook:
              response["data"] = handleRunHook(data);
              break;
            case +PrivilegedMethod::checkHook:
              response["data"] = handleCheckHookExists(data);
              break;
            case +PrivilegedMethod::resolveToIp:
              response["data"] = handleResolveToIp(data);
              break;
          }

          response["query_id"] = query_id;

          std::string txBuffer = response.dump(0);

          if(send(fd, txBuffer.data(), txBuffer.size() + 1, 0) < 0) {
            perror("send");
            exit(1);
          }
        },
        "privHandler");
  }
}
