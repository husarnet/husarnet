// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/config_storage.h"
#include <stdio.h>
#include <iostream>
#include "husarnet/util.h"

#define HOST_TABLE_KEY "host-table"
#define WHITELIST_KEY "whitelist"
#define INTERNAL_SETTINGS_KEY "internal-settings"
#define USER_SETTINGS_KEY "user-settings"

ConfigStorage::ConfigStorage(
    std::function<std::string()> readFunc,
    std::function<void(std::string)> writeFunc,
    std::map<UserSetting, std::string> userDefaults,
    std::map<UserSetting, std::string> userOverrides,
    std::map<InternalSetting, std::string> internalDefaults)
    : readFunc(readFunc),
      writeFunc(writeFunc),
      userDefaults(userDefaults),
      userOverrides(userOverrides),
      internalDefaults(internalDefaults)
{
  deserialize(readFunc());
}

std::string ConfigStorage::serialize()
{
  // TODO make this ignore pretty formatting on space constrained platforms like
  // ESP32
  return this->currentData.dump(4);
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

  writeFunc(serialize());
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

  save();
}

void ConfigStorage::hostTableRm(std::string hostname)
{
  currentData[HOST_TABLE_KEY].erase(hostname);
  save();
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
  save();
}

void ConfigStorage::whitelistAdd(IpAddress address)
{
  if(isOnWhitelist(address)) {
    return;
  }

  currentData[WHITELIST_KEY] += address.toString();
  save();
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
}

bool ConfigStorage::isOnWhitelist(IpAddress address)
{
  auto strAddr = address.toString();

  for(const auto& item : currentData[WHITELIST_KEY]) {
    if(strAddr == item) {
      return true;
    }
  }
  return false;
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

void ConfigStorage::setInternalSetting(InternalSetting setting, bool value)
{
  setInternalSetting(setting, value ? trueValue : "");
}

void ConfigStorage::setInternalSetting(InternalSetting setting, int value)
{
  setInternalSetting(setting, std::to_string(value));
}

std::string ConfigStorage::getInternalSetting(InternalSetting setting)
{
  auto settingStr = setting._to_string();
  if(currentData[INTERNAL_SETTINGS_KEY].contains(settingStr)) {
    return currentData[INTERNAL_SETTINGS_KEY][settingStr];
  }
  if(internalDefaults.contains(setting)) {
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

void ConfigStorage::setUserSetting(UserSetting setting, bool value)
{
  setUserSetting(setting, value ? trueValue : "");
}

void ConfigStorage::setUserSetting(UserSetting setting, int value)
{
  setUserSetting(setting, std::to_string(value));
}

std::string ConfigStorage::getUserSetting(UserSetting setting)
{
  auto settingStr = setting._to_string();
  if(userOverrides.contains(setting)) {
    return userOverrides[setting];
  }
  if(currentData[USER_SETTINGS_KEY].contains(settingStr)) {
    return currentData[USER_SETTINGS_KEY][settingStr];
  }
  if(userDefaults.contains(setting)) {
    return userDefaults[setting];
  }

  return "";
}

bool ConfigStorage::getUserSettingBool(UserSetting setting)
{
  return getUserSetting(setting) == trueValue;
}

int ConfigStorage::getUserSettingInt(UserSetting setting)
{
  return stoi(getUserSetting(setting));
}

std::map<std::string, std::string> ConfigStorage::getUserSettings()
{
  std::map<std::string, std::string> allSettings;

  for(auto& setting : UserSetting::_values()) {
    allSettings[setting._to_string()] = getUserSetting(setting);
  }

  return allSettings;
}

extern char** environ;

std::map<UserSetting, std::string> getEnvironmentOverrides()
{
  std::map<UserSetting, std::string> result;
  for(char** environ_ptr = environ; *environ_ptr != nullptr; environ_ptr++) {
    std::string envUpper = strToUpper(std::string(*environ_ptr));
    std::string prefix = "HUSARNET_";

    if(!startswith(envUpper, prefix)) {
      continue;
    }

    std::vector<std::string> splitted = split(*environ_ptr, '=', 1);
    if(splitted.size() == 1) {
      continue;
    }

    std::string keyName = strToLower(splitted[0].substr(prefix.size()));
    auto keyOptional = UserSetting::_from_string_nothrow(keyName.c_str());
    if(!keyOptional) {
      continue;
    }

    auto key = *keyOptional;
    std::string value = splitted[1];

    result[key] = value;
  }

  return result;
}
