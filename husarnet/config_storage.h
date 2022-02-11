#pragma once
#include <functional>
#include <list>
#include <stdexcept>
#include "enum.h"
#include "nlohmann/json.hpp"
#include "util.h"

using namespace nlohmann;  // json

// @TODO
// rozkminić czy da się to scalić z settings (network id = global/daemon/manual
// i wyjebane?)

// rozkminić czy da się zrobić osobną, globalną tabelę od defaultów
// (zdefiniowaną w husarnet_config.h?) dla "nietypowych" danych takich jak np.
// ustawienia demona, adres dashboardu, etc

// rozkminić że niektóre akcje powinny być mocno poblokowane/powodować crash
// jeśli zrobione bez unlocka - typu zmiana adresu dashbaorda

// protect writes with gil I guess

// wywalić obsługę klucza master

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
  ConfigStorage(std::function<std::string()> readFunc,
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

  void setUserSetting(UserSetting setting, std::string value);
  std::string getUserSetting(UserSetting setting);
};
