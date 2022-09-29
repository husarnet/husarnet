// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <list>
#include <map>
#include <string>

#include <stdint.h>

#include "husarnet/config_storage.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"

class HusarnetManager;
class TunTap;
class UpperLayer;
class UserSetting;

using Time = int64_t;

namespace Port {
  void init();  // Should be called as early as possible

  // TODO ympek: getThreadName() exists only in Windows port
  // good idea would be to unify this
  const char* getThreadName();
  void startThread(
      std::function<void()> func,
      const char* name,
      int stack = -1,
      int priority = 2);

  IpAddress resolveToIp(std::string hostname);

  Time getCurrentTime();  // some monotonic time in ms

  UpperLayer* startTunTap(HusarnetManager* manager);

  std::map<UserSetting, std::string> getEnvironmentOverrides();

  std::string readFile(std::string path);
  bool writeFile(std::string path, std::string content);
  bool isFile(std::string path);
  bool renameFile(std::string src, std::string dst);
  bool removeFile(std::string path);

  void notifyReady();
}  // namespace Port
