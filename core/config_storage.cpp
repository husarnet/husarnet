// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/config_storage.h"

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <map>
#include <utility>

#include "husarnet/husarnet_manager.h"
#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#define HOST_TABLE_KEY "host-table"
#define WHITELIST_KEY "whitelist"
#define INTERNAL_SETTINGS_KEY "internal-settings"
#define USER_SETTINGS_KEY "user-settings"

ConfigStorage::ConfigStorage(
    HusarnetManager* manager,
    std::function<std::string()> readFunc,
    std::function<void(std::string)> writeFunc,
    std::map<UserSetting, std::string> userDefaults,
    std::map<UserSetting, std::string> userOverrides,
    std::map<InternalSetting, std::string> internalDefaults)
    : manager(manager),
      readFunc(readFunc),
      writeFunc(writeFunc),
      userDefaults(userDefaults),
      userOverrides(userOverrides),
      internalDefaults(internalDefaults)
{
  deserialize(readFunc());
}

std::string ConfigStorage::serialize()
{
#ifdef PORT_ESP32
  return this->currentData.dump(0);
#else
  return this->currentData.dump(4);
#endif
}

void ConfigStorage::deserialize(std::string blob)
{
  // Initialize an empty file with something JSON-ish
  if(blob.length() < 3) {
    blob = "{}";
  }

  this->currentData = json::parse(blob);

  // Make sure that all data is in a proper format
  if(!currentData[HOST_TABLE_KEY].is_object()) {
    currentData[HOST_TABLE_KEY] = json::object();
  }
  if(!currentData[WHITELIST_KEY].is_array()) {
    currentData[WHITELIST_KEY] = json::array();
  }
  if(!currentData[INTERNAL_SETTINGS_KEY].is_object()) {
    currentData[INTERNAL_SETTINGS_KEY] = json::object();
  }
  if(!currentData[USER_SETTINGS_KEY].is_object()) {
    currentData[USER_SETTINGS_KEY] = json::object();
  }
}

void ConfigStorage::save()
{
  if(!this->shouldSaveImmediately) {
    return;
  }

  manager->getHooksManager()->withRw([&]() {
    LOG_DEBUG("saving settings for ConfigStorage");
    writeFunc(serialize());
    updateHostsInSystem();
  });
}

void ConfigStorage::updateHostsInSystem()
{
  if(hostCacheInvalidated) {
    manager->updateHosts();
  }

  hostCacheInvalidated = false;
}

json ConfigStorage::getCurrentData()
{
  return currentData;
}

void ConfigStorage::groupChanges(std::function<void()> f)
{
  this->shouldSaveImmediately = false;
  f();
  this->shouldSaveImmediately = true;
  save();
}

void ConfigStorage::hostTableAdd(std::string hostname, IpAddress address)
{
  currentData[HOST_TABLE_KEY][hostname] = address.toString();

  hostCacheInvalidated = true;
  save();
  manager->getHooksManager()->runHook(HookType::hosttable_changed);
}

void ConfigStorage::hostTableRm(std::string hostname)
{
  currentData[HOST_TABLE_KEY].erase(hostname);

  hostCacheInvalidated = true;
  save();
  manager->getHooksManager()->runHook(HookType::hosttable_changed);
}

std::map<std::string, IpAddress> ConfigStorage::getHostTable()
{
  std::map<std::string, IpAddress> r;

  for(const auto& item :
      currentData[HOST_TABLE_KEY].get<std::map<std::string, std::string>>()) {
    r[item.first] = IpAddress::parse(item.second);
  }

  return r;
}

void ConfigStorage::hostTableClear()
{
  currentData[HOST_TABLE_KEY].clear();

  hostCacheInvalidated = true;
  save();
}

void ConfigStorage::whitelistAdd(IpAddress address)
{
  if(isOnWhitelist(address)) {
    return;
  }

  currentData[WHITELIST_KEY] += address.toString();
  save();
  manager->getHooksManager()->runHook(HookType::whitelist_changed);
}

void ConfigStorage::whitelistRm(IpAddress address)
{
  auto strAddr = address.toString();
  bool changed = true;

  // This will remove all occurences
  while(changed) {
    changed = false;
    int i = 0;
    for(const auto& item : currentData[WHITELIST_KEY]) {
      if(strAddr == item) {
        currentData[WHITELIST_KEY].erase(i);
        changed = true;
        break;
      }
      i++;
    }
  }

  save();
  manager->getHooksManager()->runHook(HookType::whitelist_changed);
}

bool ConfigStorage::isOnWhitelist(IpAddress address)
{
  // TODO long term - it'd be wise to optimize it (use hashmaps or sth)
  auto whitelist = currentData[WHITELIST_KEY];
  return std::find(begin(whitelist), end(whitelist), address.str()) !=
         std::end(whitelist);
}

std::list<IpAddress> ConfigStorage::getWhitelist()
{
  std::list<IpAddress> r;

  for(const auto& item : currentData[WHITELIST_KEY]) {
    r.push_back(IpAddress::parse(item));
  }

  return r;
}

void ConfigStorage::whitelistClear()
{
  currentData[WHITELIST_KEY].clear();
  save();
}

void ConfigStorage::setInternalSetting(
    InternalSetting setting,
    std::string value)
{
  currentData[INTERNAL_SETTINGS_KEY][setting._to_string()] = value;
  save();
}

void ConfigStorage::setInternalSetting(
    InternalSetting setting,
    const char* value)
{
  setInternalSetting(setting, static_cast<std::string>(value));
}

void ConfigStorage::setInternalSetting(InternalSetting setting, bool value)
{
  setInternalSetting(setting, value ? trueValue : "");
}

void ConfigStorage::setInternalSetting(InternalSetting setting, int value)
{
  setInternalSetting(setting, std::to_string(value));
}

void ConfigStorage::clearInternalSetting(InternalSetting setting)
{
  currentData[INTERNAL_SETTINGS_KEY].erase(setting._to_string());
  save();
}

std::string ConfigStorage::getInternalSetting(InternalSetting setting)
{
  auto settingStr = setting._to_string();
  if(currentData[INTERNAL_SETTINGS_KEY].contains(settingStr)) {
    return currentData[INTERNAL_SETTINGS_KEY][settingStr];
  }
  if(mapContains(internalDefaults, setting)) {
    return internalDefaults[setting];
  }

  return "";
}

bool ConfigStorage::getInternalSettingBool(InternalSetting setting)
{
  return getInternalSetting(setting) == trueValue;
}

int ConfigStorage::getInternalSettingInt(InternalSetting setting)
{
  if(getInternalSetting(setting) == "") {
    return 0;
  }

  return stoi(getInternalSetting(setting));
}

bool ConfigStorage::isInternalSettingEmpty(InternalSetting setting)
{
  auto settingStr = setting._to_string();
  if(!currentData[INTERNAL_SETTINGS_KEY].contains(settingStr)) {
    return true;
  }

  if(getInternalSetting(setting).empty()) {
    return true;
  }

  return false;
}

void ConfigStorage::setUserSetting(UserSetting setting, std::string value)
{
  currentData[USER_SETTINGS_KEY][setting._to_string()] = value;
  save();
}

void ConfigStorage::setUserSetting(UserSetting setting, const char* value)
{
  setUserSetting(setting, static_cast<std::string>(value));
}

void ConfigStorage::setUserSetting(UserSetting setting, bool value)
{
  setUserSetting(setting, value ? trueValue : "");
}

void ConfigStorage::setUserSetting(UserSetting setting, int value)
{
  setUserSetting(setting, std::to_string(value));
}

void ConfigStorage::setUserSetting(UserSetting setting, InetAddress value)
{
  setUserSetting(setting, value.str());
}

void ConfigStorage::clearUserSetting(UserSetting setting)
{
  currentData[USER_SETTINGS_KEY].erase(setting._to_string());
  save();
}

bool ConfigStorage::isUserSettingOverriden(UserSetting setting)
{
  if(!mapContains(userOverrides, setting)) {
    return false;
  }

  if(getPersistentUserSetting(setting) == getUserSetting(setting)) {
    return false;
  }

  return true;
}

std::string ConfigStorage::getPersistentUserSetting(UserSetting setting)
{
  auto settingStr = setting._to_string();
  if(currentData[USER_SETTINGS_KEY].contains(settingStr)) {
    return currentData[USER_SETTINGS_KEY][settingStr];
  }
  if(mapContains(userDefaults, setting)) {
    return userDefaults[setting];
  }

  return "";
}

void ConfigStorage::persistUserSettingOverride(UserSetting setting)
{
  if(!isUserSettingOverriden(setting)) {
    return;
  }

  // This only looks like a no-op. In practice it reads the temporarily
  // overriden setting and persistently stores it's value
  setUserSetting(setting, getUserSetting(setting));
}

std::string ConfigStorage::getUserSetting(UserSetting setting)
{
  if(mapContains(userOverrides, setting)) {
    return userOverrides[setting];
  }

  return getPersistentUserSetting(setting);
}

bool ConfigStorage::getUserSettingBool(UserSetting setting)
{
  return getUserSetting(setting) == trueValue;
}

int ConfigStorage::getUserSettingInt(UserSetting setting)
{
  if(getUserSetting(setting) == "") {
    return 0;
  }

  return stoi(getUserSetting(setting));
}

InetAddress ConfigStorage::getUserSettingInet(UserSetting setting)
{
  return InetAddress::parse(getUserSetting(setting));
}

IpAddress ConfigStorage::getUserSettingIp(UserSetting setting)
{
  return IpAddress::parse(getUserSetting(setting));
}

std::map<std::string, std::string> ConfigStorage::getUserSettings()
{
  std::map<std::string, std::string> allSettings;

  for(auto& setting : UserSetting::_values()) {
    allSettings[setting._to_string()] = getUserSetting(setting);
  }

  return allSettings;
}
