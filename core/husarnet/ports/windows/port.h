// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <cassert>
#include <cstdlib>
#ifdef _WIN32
#include <winsock2.h>

#include <iphlpapi.h>
#include <mswsock.h>
#include <winioctl.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#undef IGNORE  // ...
#undef ERROR   // Windows API macro pollution...
#endif
#include <string>

#include "husarnet/ports/dummy_task_priorities.h"

#include "tun.h"

extern errno_t rand_s(unsigned int* randomValue);
inline void sleep(int sec)
{
  Sleep(sec * 1000);
}
#define SOCKFUNC(name) ::name
#define SOCKFUNC_close closesocket

inline long random()
{
  unsigned int res = 0;
  rand_s(&res);
  return res;
}

inline int renameFile(const char* src, const char* dst)
{
  return MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
}

void fixPermissions(std::string path);

namespace Port {
  int callWindowsCmd(std::string cmd);
  void insertFirewallRule(const std::string& firewallRuleName);
  void deleteFirewallRules(const std::string& firewallRuleName);

  void setupRoutingTable(const std::string& netshName);
  void setupFirewall(const std::string& netshName);
}

