// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <list>
#include <stdexcept>
#include <string>

#include "husarnet/config_storage.h"
#include "husarnet/ipaddress.h"

#include "enum.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

// Remember to add sane defaults in husarnet_config.h
// Keep the naming consistent, i.e use only enable* format, and *not* *enable or
// disable*
BETTER_ENUM(
    InternalSetting,
    int,
    websetupSecret = 1  // string
)
BETTER_ENUM(
    UserSetting,
    int,
    dashboardFqdn = 1,            // string
    enableWhitelist = 2,          // bool
    interfaceName = 3,            // string
    daemonApiPort = 4,            // int
    enableCompression = 5,        // bool
    enableUdpTunelling = 6,       // bool
    enableTcpTunelling = 7,       // bool
    enableUdp = 8,                // bool
    enableMulticast = 9,          // bool
    overrideBaseAddress = 10,     // inet
    overrideSourcePort = 11,      // int
    extraAdvertisedAddress = 12,  // inet
    joinCode = 13,                // string, this will never be persisted
    hostname = 14,                // string, this will never be persisted
    logVerbosity = 16             // int
)

const std::string trueValue = "true";
const std::string falseValue =
    "false";  // This is not strictly checked. It's here just to provide a known
              // placeholder

class HusarnetManager;

class ConfigStorage {
  HusarnetManager* manager;
  std::function<std::string()> readFunc;
  std::function<void(std::string)> writeFunc;
  std::map<UserSetting, std::string> userDefaults;
  std::map<UserSetting, std::string> userOverrides;
  std::map<InternalSetting, std::string> internalDefaults;

  json currentData;

  bool shouldSaveImmediately = true;
  bool hostCacheInvalidated = true;

  std::string serialize();
  void deserialize(std::string blob);

  void save();

 public:
  ConfigStorage(
      HusarnetManager* manager,
      std::function<std::string()> readFunc,
      std::function<void(std::string)> writeFunc,
      std::map<UserSetting, std::string> userDefaults,
      std::map<UserSetting, std::string> userOverrides,
      std::map<InternalSetting, std::string> internalDefaults);

  ConfigStorage(ConfigStorage&) = delete;

  void updateHostsInSystem();

  json getCurrentData();

  void groupChanges(std::function<void()> f);

  void hostTableAdd(std::string hostname, IpAddress address);
  void hostTableRm(std::string hostname);
  std::map<std::string, IpAddress> getHostTable();
  void hostTableClear();

  void whitelistAdd(IpAddress address);
  void whitelistRm(IpAddress address);
  bool isOnWhitelist(IpAddress address);
  std::list<IpAddress> getWhitelist();
  void whitelistClear();

  void setInternalSetting(InternalSetting setting, std::string value);
  void setInternalSetting(InternalSetting setting, const char* value);
  void setInternalSetting(InternalSetting setting, bool value);
  void setInternalSetting(InternalSetting setting, int value);
  void clearInternalSetting(InternalSetting setting);

  std::string getInternalSetting(InternalSetting setting);
  bool getInternalSettingBool(InternalSetting setting);
  int getInternalSettingInt(InternalSetting setting);

  bool isInternalSettingEmpty(InternalSetting setting);

  void setUserSetting(UserSetting setting, std::string value);
  void setUserSetting(UserSetting setting, const char* value);
  void setUserSetting(UserSetting setting, bool value);
  void setUserSetting(UserSetting setting, int value);
  void setUserSetting(UserSetting setting, InetAddress inet);
  void clearUserSetting(UserSetting setting);

  bool isUserSettingOverriden(UserSetting setting);
  std::string getPersistentUserSetting(UserSetting setting);
  void persistUserSettingOverride(UserSetting setting);

  std::string getUserSetting(UserSetting setting);
  bool getUserSettingBool(UserSetting setting);
  int getUserSettingInt(UserSetting setting);
  InetAddress getUserSettingInet(UserSetting setting);

  std::map<std::string, std::string> getUserSettings();
};
