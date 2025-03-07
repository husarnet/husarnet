// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/config_manager.h"

#include <etl/string.h>
#include <sockets.h>

#include "husarnet/husarnet_config.h"
#include "husarnet/licensing.h"

#include "ngsocket_messages.h"

ConfigManager::ConfigManager(
    const HooksManager* hooksManager,
    const ConfigEnv* configEnv)
    : hooksManager(hooksManager), configEnv(configEnv)
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

  // TODO: mutex is needed
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
  auto contents = Port::readStorage(StorageKey::config);
  if(contents.empty()) {
    LOG_INFO("saved config.json is empty/nonexistent");
    return false;
  }
  auto configJson = json::parse(contents, nullptr, false);
  if(configJson.is_discarded()) {
    LOG_INFO("saved config.json is not a valid JSON, not reading");
    return false;
  }
  // TODO: mutex here
  this->config_json = configJson;
  return true;
}

bool ConfigManager::readCache()
{
  auto contents = Port::readStorage(StorageKey::cache);
  if(contents.empty()) {
    LOG_INFO("saved cache.json is empty/nonexistent");
    return false;
  }
  auto cacheJson = json::parse(contents, nullptr, false);
  if(cacheJson.is_discarded()) {
    LOG_INFO("saved cache.json is not a valid JSON, not reading");
    return false;
  }
  // TODO: mutex here
  this->cache_json = cacheJson;
  return true;
}

bool ConfigManager::isPeerAllowed(const HusarnetAddress& address) const
{
  // TODO: check ENABLE_CONTROL_PLANE flag I think
  // first check our iPS, then whitelist, use efficient data str (eg etl::map)
  const auto ebServers = this->cache_json["license"][LICENSE_EB_SERVERS_KEY]
                       .get<std::vector<std::string>>();
  return true;
}

etl::vector<HusarnetAddress, EVENTBUS_ADDRESSES_LIMIT>
ConfigManager::getEventbusAddresses() const
{
  const auto ips = this->cache_json["license"][LICENSE_EB_SERVERS_KEY]
                       .get<std::vector<std::string>>();
  auto result = etl::vector<HusarnetAddress, EVENTBUS_ADDRESSES_LIMIT>();
  for (auto& ip: ips) {
    result.push_back(HusarnetAddress::parse(ip));
  }
  return result;
}

etl::vector<HusarnetAddress, DASHBOARD_API_ADDRESSES_LIMIT>
ConfigManager::getDashboardApiAddresses() const
{
  const auto ips = this->cache_json["license"][LICENSE_API_SERVERS_KEY]
                       .get<std::vector<std::string>>();
  auto result = etl::vector<HusarnetAddress, DASHBOARD_API_ADDRESSES_LIMIT>();
  for (auto& ip: ips) {
    result.push_back(HusarnetAddress::parse(ip));
  }
  return result;
}

etl::vector<InternetAddress, BASE_ADDRESSES_LIMIT>
ConfigManager::getBaseAddresses() const
{
  const auto ips =
      this->cache_json[CACHE_LICENSE][LICENSE_BASE_SERVER_ADDRESSES_KEY]
          .get<std::vector<std::string>>();
  auto result = etl::vector<HusarnetAddress, BASE_ADDRESSES_LIMIT>();
  for (auto& ip: ips) {
    result.push_back(HusarnetAddress::parse(ip));
  }
  return result;
}

etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT>
ConfigManager::getMulticastDestinations(HusarnetAddress id)
{
  // TODO: figure out if this check was even relevant
  //  if(!id == deviceIdFromIpAddress(multicastDestination)) {
  //    return {};
  //  }

  auto result = etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT>();
  for(auto& peer : this->config_json[CONFIG_PEERS_KEY]) {
    auto addr = peer["address"].get<std::string>();
    result.push_back(HusarnetAddress::parse(addr));
  }
  return result;
}
const json ConfigManager::getStatus() const
{
  // TODO: there's much more to this, implement it later
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
  return Port::writeStorage(StorageKey::config, config_json.dump(JSON_INDENT_SPACES));
}

bool ConfigManager::writeCache()
{
  return Port::writeStorage(StorageKey::cache, config_json.dump(JSON_INDENT_SPACES));
}

[[noreturn]] void ConfigManager::periodicThread()
{
  while(true) {
    LOG_INFO("ConfigManagerDev: periodic thread started")
    this->mutex.lock();
    if(this->readConfig()) {
      LOG_INFO("ConfigManagerDev: config read from disk successful")
    }
    if(this->readCache()) {
      LOG_INFO("ConfigManagerDev: cache read from disk successful")
    }

    LOG_INFO("ConfigManagerDev: getting the license")
    this->getLicense();
    this->flush();

    this->mutex.unlock();
    LOG_INFO("ConfigManagerDev: periodic thread finished")
    Port::threadSleep(configManagerThreadPeriodMs);
  }
}

void ConfigManager::waitInit()
{
  LOG_INFO("ConfigManagerDev: wait init started")
  // we need to at least have license information, like for example base server
  // to connect to.
  while(this->cache_json["license"].is_discarded() ||
        this->cache_json["license"].is_null()) {
    // busy wait
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
  // some sort of IPC is here needed
}
