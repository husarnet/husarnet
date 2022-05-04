// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <list>
#include "husarnet/ipaddress.h"

namespace Port {
  void init();  // Should be called as early as possible

  void startThread(
      std::function<void()> func,
      const char* name,
      int stack = -1,
      int priority = 2);

  IpAddress resolveToIp(std::string hostname);

  int64_t getCurrentTime();

  void startTunTap(std::string name);
}  // namespace Port
