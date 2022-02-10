#pragma once
#include <functional>
#include <list>
#include <stdexcept>
#include "enum.h"
#include "nlohmann/json.hpp"
#include "util.h"

using namespace nlohmann;  // json

// @TODO
// wywalić wersję sqlite razem z deps
// scalić interface z wersją inmemory
// zamienić structy na legitne klasy dla czytelności na c++
// zmienić api żeby korzystało z write/read ConfigTableBlob z privileged
// interfejsu zmienić api żeby wysokopoziomowo nie wchodzić w interakcje z
// blobami w ogóle, ale było na tyle wysoko żeby testy miały jak

// a co jakby zrobić jakieś typowe "get scalar" i "get list" metody i
// zrezygnować z inner-most poziomu?

// rozkminić czy da się to scalić z settings (network id = global/daemon/manual
// i wyjebane?)

// settings zrobić po enumach a nie po stringach

// metody od settings zrobić 1 level bardziej płaskie, th ukryć nazwę klucza
// używanego dla tych danych

// rozkminić czy da się zrobić osobną, globalną tabelę od defaultów
// (zdefiniowaną w husarnet_config.h?) dla "nietypowych" danych takich jak np.
// ustawienia demona, adres dashboardu, etc

// rozkminić że niektóre akcje powinny być mocno poblokowane/powodować crash
// jeśli zrobione bez unlocka - typu zmiana adresu dashbaorda

// zmienić api do zapisywania/flushowania zapisów na wartownika przed return w
// każdej operacji write który sprawdza zmienną globalną dla klasy shouldWrite,
// jeśli tak to pisze jeśli nei to nie zmienić nazwę runintransaction na coś
// typu flushMultiple które najpierw zrobi shouldWrite=False, potem f(), na
// końcu shouldWrite=True i ręcznie wywoła wartownika

// na końcu rozkminić czy nie zmienic formatu przechowywanych danych na zwykłego
// jsona (największy problem to esp32 tutaj - sprawdzić czy port to pociągnie)

// protect writes with gil I guess

// wywalić obsługę klucza master

// rozważyć rename na configstorage żeby było jednolicie

// rozważyć porobienie dedykowanych api do konkretnych czynności (typu właśnie
// zarządzanie host listą, whitelistą,…)

BETTER_ENUM(InternalSetting, int, websetupSecret = 1)
BETTER_ENUM(UserSetting, int, dashboardUrl = 1)

class ConfigStorage {
  std::function<std::string()> readFunc;
  std::function<void(std::string)> writeFunc;
  std::map<UserSetting, std::string> settingsDefaults;
  std::map<UserSetting, std::string> settingsEnvOverrides;

  json currentData;

  bool shouldSaveImmediately = true;

  std::string serialize();
  void deserialize(std::string blob);

  void save();

 public:
  ConfigStorage(std::function<std::string()> readFunc,
                std::function<void(std::string)> writeFunc,
                std::map<UserSetting, std::string> settingsDefaults,
                std::map<UserSetting, std::string> settingsEnvOverrides);

  ConfigStorage(ConfigStorage&) = delete;

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
  void clearInternalSetting(InternalSetting setting);

  void setUserSetting(UserSetting setting, std::string value);
  std::string getUserSetting(UserSetting setting);
  void clearUserSetting(UserSetting setting);

  void groupChanges(std::function<void()> f);
};
