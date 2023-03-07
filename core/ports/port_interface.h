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

  void startThread(
      std::function<void()> func,
      const char* name,
      int stack = -1,
      int priority = 2);

  IpAddress resolveToIp(const std::string& hostname);

  Time getCurrentTime();  // some monotonic time in ms

  UpperLayer* startTunTap(HusarnetManager* manager);

  std::map<UserSetting, std::string> getEnvironmentOverrides();

  std::string readFile(const std::string& path);
  bool writeFile(const std::string& path, const std::string& content);
  bool isFile(const std::string& path);
  bool renameFile(const std::string& src, const std::string& dst);

  // TODO why this is doubled into both PortInterface and PrivilegedInterface?
  void notifyReady();

  void log(const std::string& message);

  void runScripts(std::string path);
  bool checkScriptsExist(std::string path);
}  // namespace Port
