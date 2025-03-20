// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <list>
#include <map>
#include <string>

#include <stdint.h>

#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/logging.h"

#include "etl/map.h"
#include "nlohmann/json.hpp"

using Time = int64_t;

enum class EnvKey
{
  tldFqdn,
  logVerbosity,
  enableHooks,
  enableControlPlane,
  daemonInterface,
  daemonApiInterface,
  daemonApiHost,
  daemonApiPort,
};

#define ENV_KEY_OPTIONS 9

enum class StorageKey
{
  id,
  config,
  cache,
  daemonApiToken,
};

#define STORAGE_KEY_OPTIONS 4

enum class HookType
{
  claimed,
};

#define HOOK_TYPE_OPTIONS 1

namespace Port {
  void init();                // This is called from HusarnetManager
  void die(std::string msg);  // Die with an error

#ifdef PORT_FAT
  void fatInit();  // Call this from init() on fat platforms
#endif

  // Basic interfaces
  void threadStart(std::function<void()> func, const char* name, int stack = -1, int priority = 2);
  void threadSleep(Time ms);

  etl::map<EnvKey, std::string, ENV_KEY_OPTIONS> getEnvironmentOverrides();

  void notifyReady();

  // Time and logging
  void log(const LogLevel level, const std::string& message);

  Time getCurrentTime();             // some monotonic time in ms
  const std::string getHumanTime();  // human readable time

  // Interfaces
  IpAddress getIpAddressFromInterfaceName(const std::string& interfaceName);
  std::vector<IpAddress> getLocalAddresses();

  UpperLayer* startTunTap(const HusarnetAddress& myAddress, std::string interfaceName);

  void processSocketEvents(void* tuntap);

  // Hostnames
  std::string getSelfHostname();
  bool setSelfHostname(const std::string& newHostname);

  void updateHostsFile(const std::map<std::string, HusarnetAddress>& data);

  // DNS
  IpAddress resolveToIp(const std::string& hostname);

  // Hooks
  bool runHook(HookType hookType);

  // Layer 7 network ops
  struct HttpResult {
    int statusCode;  // -1 will be set if the request failed
    std::string bytes;
  };
  HttpResult httpGet(const std::string& url, const std::string& path);
  HttpResult httpPost(const std::string& url, const std::string& path, const std::string& body);

  // Storage
  std::string readStorage(StorageKey key);  // Empty means error (or actually empty)
  bool writeStorage(StorageKey key, const std::string& data);

}  // namespace Port
