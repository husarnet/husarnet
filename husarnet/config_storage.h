// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <list>
#include <stdexcept>
#include "enum.h"
#include "husarnet/ipaddress.h"
#include "husarnet/util.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

// TODO rozkminić czy da się to scalić z settings (network id =
// global/daemon/manual i wyjebane?)

// TODO rozkminić że niektóre akcje powinny być mocno poblokowane/powodować
// crash jeśli zrobione bez unlocka - typu zmiana adresu dashbaorda

// TODO protect writes with gil I guess

BETTER_ENUM(InternalSetting, int, websetupSecret = 1)
BETTER_ENUM(
    UserSetting,
    int,
    dashboardUrl = 1,
    whitelistEnable = 2,
    interfaceName = 3,
    apiPort = 4)

const std::string trueValue = "true";

class ConfigStorage {
  std::function<std::string()> readFunc;
  std::function<void(std::string)> writeFunc;
  std::map<UserSetting, std::string> userDefaults;
  std::map<UserSetting, std::string> userOverrides;
  std::map<InternalSetting, std::string> internalDefaults;

  json currentData;

  bool shouldSaveImmediately = true;

  std::string serialize();
  void deserialize(std::string blob);

  void save();

 public:
  ConfigStorage(
      std::function<std::string()> readFunc,
      std::function<void(std::string)> writeFunc,
      std::map<UserSetting, std::string> userDefaults,
      std::map<UserSetting, std::string> userOverrides,
      std::map<InternalSetting, std::string> internalDefaults);

  ConfigStorage(ConfigStorage&) = delete;

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
  void setInternalSetting(InternalSetting setting, bool value);
  void setInternalSetting(InternalSetting setting, int value);

  std::string getInternalSetting(InternalSetting setting);
  bool getInternalSettingBool(InternalSetting setting);
  int getInternalSettingInt(InternalSetting setting);

  bool isInternalSettingEmpty(InternalSetting setting);

  void setUserSetting(UserSetting setting, std::string value);
  void setUserSetting(UserSetting setting, bool value);
  void setUserSetting(UserSetting setting, int value);

  std::string getUserSetting(UserSetting setting);
  bool getUserSettingBool(UserSetting setting);
  int getUserSettingInt(UserSetting setting);

  std::map<std::string, std::string> getUserSettings();
};

std::map<UserSetting, std::string> getEnvironmentOverrides();
