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

using namespace nlohmann;

// TODO rozkminić czy da się to scalić z settings (network id =
// global/daemon/manual i wyjebane?)

// TODO rozkminić że niektóre akcje powinny być mocno poblokowane/powodować
// crash jeśli zrobione bez unlocka - typu zmiana adresu dashbaorda

// TODO protect writes with gil I guess

// TODO wywalić obsługę klucza master

BETTER_ENUM(InternalSetting, int, websetupSecret = 1)
BETTER_ENUM(UserSetting, int, dashboardUrl = 1)

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
  std::string getInternalSetting(InternalSetting setting);
  // TODO add getInternalSettingInt

  void setUserSetting(UserSetting setting, std::string value);
  std::string getUserSetting(UserSetting setting);
  // TODO add getUserSettingInt
};

std::map<UserSetting, std::string> getEnvironmentOverrides();
