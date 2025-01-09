// Copyright (c) 2025 Husarnet sp. z o.o.
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
#include "husarnet/logging.h"

class HusarnetManager;
class TunTap;
class UpperLayer;
class UserSetting;

using Time = int64_t;

namespace Port {
  void init();  // This is called from HusarnetManager

#ifdef PORT_FAT
  void fatInit();  // Call this from init() on fat platforms
#endif

  // Basic interfaces
  void startThread(
      std::function<void()> func,
      const char* name,
      int stack = -1,
      int priority = 2);

  std::map<UserSetting, std::string> getEnvironmentOverrides();

  void notifyReady();

  // Time and logging
  void log(const LogLevel level, const std::string& message);

  Time getCurrentTime();             // some monotonic time in ms
  const std::string getHumanTime();  // human readable time

  // Interfaces
  IpAddress getIpAddressFromInterfaceName(const std::string& interfaceName);
  std::vector<IpAddress> getLocalAddresses();

  UpperLayer* startTunTap(HusarnetManager* manager);

  void processSocketEvents(HusarnetManager* manager);

  // Hostnames
  std::string getSelfHostname();
  bool setSelfHostname(const std::string& newHostname);

  void updateHostsFile(const std::map<std::string, IpAddress>& data);

  // DNS
  IpAddress resolveToIp(const std::string& hostname);

  // Hooks
  // TODO rename those to runHooks and checkHooksExist
  // TODO make them accept eventName instead of a "path relative to config dir"
  // TODO also refactor this so listScripts(listHooks) gives a list of
  // eventNames that have scripts available
  bool runScripts(const std::string& eventName);
  bool checkScriptsExist(const std::string& eventName);

  // Storage
  std::string getConfigDir();

  std::string readIdentity();
  bool writeIdentity(const std::string&);

  std::string readConfig();
  bool writeConfig(const std::string&);

  std::string readLicenseJson();
  bool writeLicenseJson(const std::string&);

  std::string readApiSecret();
  bool writeApiSecret(const std::string&);

}  // namespace Port
