// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <list>
#include "husarnet/ipaddress.h"

class HusarnetManager;
class TunTap;

using Time = int64_t;

namespace Port {
  void init();  // Should be called as early as possible

  void startThread(
      std::function<void()> func,
      const char* name,
      int stack = -1,
      int priority = 2);

  IpAddress resolveToIp(std::string hostname);

  Time getCurrentTime();  // some monotonic time in ms

  TunTap* startTunTap(HusarnetManager* manager);
}  // namespace Port
