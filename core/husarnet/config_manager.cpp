// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/config_manager.h"

#include <etl/string.h>
#include <sockets.h>

#include "husarnet/husarnet_config.h"
#include "husarnet/licensing.h"

#include "ngsocket_messages.h"

/*

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

json retrieveCachedLicenseJson()
{
  // Don't throw an exception on parse failure
  return json::parse(Port::readLicenseJson(), nullptr, false);
}
*/

ConfigManager::ConfigManager(
    const HooksManager* hooks_manager,
    const ConfigEnv* configEnv)
    : hooks_manager(hooks_manager), configEnv(configEnv)
{
}

void ConfigManager::getLicense()
{
  auto tldAddress = this->configEnv->getTldFqdn();
  auto [statusCode, bytes] = Port::httpGet(tldAddress, "/license.json");

  if(statusCode != 200) {
    LOG_ERROR("failed to download license file (using tld address %s)", tldAddress.c_str());
    return;
  }

  auto licenseJson = json::parse(bytes);
  if(!isLicenseValid(licenseJson)) {
    LOG_CRITICAL("license file downloaded from %s is invalid", tldAddress.c_str());
    return;
  }

  this->cache_json[CACHE_LICENSE] = licenseJson;
}

HusarnetAddress ConfigManager::getCurrentApiAddress() const {
  auto addresses = this->getDashboardApiAddresses();
  return addresses[0];
}

void ConfigManager::getGetConfig()
{
  auto apiAddress = this->getCurrentApiAddress();
  auto [statusCode, bytes] = Port::httpGet(apiAddress.toString(), "/device/get_config");

  if(statusCode != 200) {
    LOG_ERROR("failed to obtain latest config (http request to %s was unsuccessful)", apiAddress.toString().c_str());
    return;
  }

  auto config = json::parse(bytes);
  this->cache_json[CACHE_GET_CONFIG] = config;
}

bool ConfigManager::readConfig()
{
  // TODO: read the file and set the structure properly
  auto configJson = json::parse(Port::readStorage(StorageKey::config));

  return false;
}
bool ConfigManager::readCache()
{
  // TODO: read the file (port)
  return false;
}

bool ConfigManager::isPeerAllowed(const HusarnetAddress& address) const
{
  // TODO: check ENABLE_CONTROL_PLANE flag I think
  // first check our iPS, then whitelist, use efficient data str (eg etl::map)
  return true;
}

const etl::array<HusarnetAddress, EVENTBUS_ADDRESSES_LIMIT>
ConfigManager::getEventbusAddresses() const
{
  const auto ips = this->cache_json["license"][LICENSE_EB_SERVERS_KEY]
                       .get<std::vector<std::string>>();
  auto result = etl::array<HusarnetAddress, EVENTBUS_ADDRESSES_LIMIT>();
  for(int i = 0; i < ips.size(); i++) {
    result[i] = HusarnetAddress::parse(ips[i]);
  }
  return result;
}

const etl::array<HusarnetAddress, DASHBOARD_API_ADDRESSES_LIMIT>
ConfigManager::getDashboardApiAddresses() const
{
  const auto ips = this->cache_json["license"][LICENSE_API_SERVERS_KEY]
                       .get<std::vector<std::string>>();
  auto result = etl::array<HusarnetAddress, DASHBOARD_API_ADDRESSES_LIMIT>();
  for(int i = 0; i < ips.size(); i++) {
    result[i] = HusarnetAddress::parse(ips[i]);
  }
  return result;
}

const etl::array<InternetAddress, BASE_ADDRESSES_LIMIT>
ConfigManager::getBaseAddresses() const
{
  const auto ips =
      this->cache_json["license"][LICENSE_BASE_SERVER_ADDRESSES_KEY]
          .get<std::vector<std::string>>();
  auto result = etl::array<InternetAddress, BASE_ADDRESSES_LIMIT>();
  for(int i = 0; i < ips.size(); i++) {
    result[i] = InternetAddress::parse(ips[i]);
  }
  return result;
}

const etl::array<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT>
ConfigManager::getMulticastDestinations(HusarnetAddress id)
{
  return etl::array<HusarnetAddress, 128>();
}
const json ConfigManager::getStatus() const
{
  return this->config_json;
}
bool ConfigManager::userWhitelistRm(const HusarnetAddress& address)
{
  return false;
}
void ConfigManager::flush()
{
  this->writeConfig();
  this->writeCache();
}

bool ConfigManager::writeConfig()
{
  return Port::writeStorage(StorageKey::config, config_json.dump(4));
}

bool ConfigManager::writeCache()
{
  return Port::writeStorage(StorageKey::cache, config_json.dump(4));
}

void ConfigManager::periodicThread()
{
  // while w srodku
  LOG_INFO("ConfigManagerDev: periodic thread started")
  if(this->readConfig()) {
    LOG_INFO("ConfigManagerDev: config read from disk successful")
  }
  if(this->readCache()) {
    LOG_INFO("ConfigManagerDev: cache read from disk successful")
  }

  LOG_INFO("ConfigManagerDev: getting the license")
  this->getLicense();
}
void ConfigManager::waitInit()
{
  LOG_INFO("ConfigManagerDev: wait init started")
  // we need to at least have license information, like for example base server
  // to connect to.
  while(this->cache_json["license"].is_discarded() ||
        this->cache_json["license"].is_null()) {
    // LOG_INFO("ConfigManagerDev: waiting")
  }
  LOG_INFO("ConfigManagerDev: wait init finished")
}
bool ConfigManager::userWhitelistAdd(const HusarnetAddress& address)
{
  auto whitelist =
      this->config_json["whitelist"].get<std::vector<std::string>>();
  return false;
}
void ConfigManager::triggerGetConfig()
{
  // send to periodicThread
}
