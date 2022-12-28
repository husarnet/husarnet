// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/shared_unix_windows/hosts_file_manipulation.h"

#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "husarnet/ports/port_interface.h"

#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#ifdef _WIN32
static std::string lineEnding = "\r\n";

static std::string getHostsFilePath()
{
  return std::string(getenv("windir")) + "/system32/drivers/etc/hosts";
}
#else
static std::string lineEnding = "\n";

static std::string getHostsFilePath()
{
  return "/etc/hosts";
}
#endif

static const std::string marker = " # managed by Husarnet";

static bool isLineMarked(std::string line)
{
  return endswith(line, marker);
}

bool updateHostsFileInternal(std::map<std::string, IpAddress> data)
{
  auto hostsFileStream = std::istringstream(Port::readFile(getHostsFilePath()));

  std::string newContents;

  // First rewrite all unmanaged lines as they are
  std::string line;
  while(std::getline(hostsFileStream, line)) {
    // getline assumes line ending is LF by default, even on Windows
    // in earlier versions of Husarnet hosts file on Windows got unwanted orphan CRs
    // here we make sure no such thing happen again AND we clean up after this bug
    // we could just add delimiter param to std::getline above, but we still have to clean up after older versions
    // so might as well leave getline as-is and just trim exceess ws.
    line = rtrim(line);
    if(!isLineMarked(line)) {
      newContents += line + lineEnding;
    }
  }

  // Then add our lines
  for(auto& [hostname, address] : data) {
    if(!validateHostname(hostname)) {
      continue;
    }
    newContents += address.toString() + " " + hostname + marker + lineEnding;
  }

  return Port::writeFile(getHostsFilePath(), newContents);
}

bool validateHostname(std::string hostname)
{
  if(hostname.size() == 0)
    return false;
  for(char c : hostname) {
    bool ok = ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
              ('0' <= c && c <= '9') || (c == '_' || c == '-' || c == '.');
    if(!ok) {
      return false;
    }
  }
  return true;
}
