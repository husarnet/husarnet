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

  this->processLicense(licenseJson);
}

void ConfigManager::processLicense(const json& licenseJson)
{
  const auto ebServerIps = licenseJson[LICENSE_EB_SERVERS_KEY]
                       .get<std::vector<std::string>>();
  this->ebAddresses.clear();
  for (auto& ipStr: ebServerIps) {
    const auto ip = HusarnetAddress::parse(ipStr);
    this->ebAddresses.push_back(ip);
    this->allowedPeers.insert(ip);
  }

  const auto apiServerIps = licenseJson[LICENSE_API_SERVERS_KEY]
                               .get<std::vector<std::string>>();
  this->apiAddresses.clear();
  for (auto& ipStr: apiServerIps) {
    const auto ip = HusarnetAddress::parse(ipStr);
    this->apiAddresses.push_back(ip);
    this->allowedPeers.insert(ip);  }

  const auto baseServerIps= licenseJson[LICENSE_BASE_SERVER_ADDRESSES_KEY]
          .get<std::vector<std::string>>();
  for (auto& ip: baseServerIps) {
    this->baseAdresses.push_back(HusarnetAddress::parse(ip));
  }
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
  // TODO: parse config to our internal structures
}

bool ConfigManager::readConfig(json& configJson)
{
  auto contents = Port::readStorage(StorageKey::config);
  if(contents.empty()) {
    LOG_INFO("saved config.json is empty/nonexistent");
    return false;
  }
  auto parsedContents = json::parse(contents, nullptr, false);
  if(parsedContents.is_discarded()) {
    LOG_INFO("saved config.json is not a valid JSON, not reading");
    return false;
  }
  configJson = parsedContents;
  return true;
}

bool ConfigManager::readCache(json& cacheJson)
{
  auto contents = Port::readStorage(StorageKey::cache);
  if(contents.empty()) {
    LOG_INFO("saved cache.json is empty/nonexistent");
    return false;
  }
  auto parsedContents = json::parse(contents, nullptr, false);
  if(parsedContents.is_discarded()) {
    LOG_INFO("saved cache.json is not a valid JSON, not reading");
    return false;
  }
  cacheJson = parsedContents;
  return true;
}

void ConfigManager::setAllowEveryone()
{
  this->mutex.lock();
  this->allowEveryone = true;
  this->mutex.unlock();
}

bool ConfigManager::isPeerAllowed(const HusarnetAddress& address) const
{
  this->mutex.lock();
  if(this->allowEveryone) {
    return true;
  }
  bool isAllowed = this->allowedPeers.contains(address);
  this->mutex.unlock();
  return isAllowed;
}

bool ConfigManager::isInfraAddress(const HusarnetAddress& address) const
{
  this->mutex.lock();
  for (auto candidate : this->apiAddresses) {
    if (candidate == address) {
      return true;
    }
  }
  for (auto candidate : this->ebAddresses) {
    if (candidate == address) {
      return true;
    }
  }
  this->mutex.unlock();
  return false;
}

const etl::vector<HusarnetAddress, EVENTBUS_ADDRESSES_LIMIT>&
ConfigManager::getEventbusAddresses() const
{
  return this->ebAddresses;
}

const etl::vector<HusarnetAddress, DASHBOARD_API_ADDRESSES_LIMIT>&
ConfigManager::getDashboardApiAddresses() const
{
  return this->apiAddresses;
}

const etl::vector<InternetAddress, BASE_ADDRESSES_LIMIT>&
ConfigManager::getBaseAddresses() const
{
  return this->baseAdresses;
}

etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT>
ConfigManager::getMulticastDestinations(HusarnetAddress id)
{
  // TODO: figure out if this check was even relevant
  //  if(!id == deviceIdFromIpAddress(multicastDestination)) {
  //    return {};
  //  }

  // TODO not sure what about infra addresses here
  auto result = etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT>();
  for(auto& peer : this->allowedPeers) {
    result.push_back(peer);
  }
  return result;
}

const json ConfigManager::getStatus() const
{
  // TODO: implement
  return json::parse("{}");
}

bool ConfigManager::userWhitelistRm(const HusarnetAddress& address)
{
  return false;
}

bool ConfigManager::writeConfig(const json& configJson)
{
  return Port::writeStorage(StorageKey::config, configJson.dump(JSON_INDENT_SPACES));
}

bool ConfigManager::writeCache(const json& cacheJson)
{
  return Port::writeStorage(StorageKey::cache, cacheJson.dump(JSON_INDENT_SPACES));
}

[[noreturn]] void ConfigManager::periodicThread()
{
  while(true) {
    this->mutex.lock();
    LOG_INFO("ConfigManagerDev: periodic thread started")
    json configJson;
    json cacheJson;

    if(this->readConfig(configJson)) {
      LOG_INFO("ConfigManagerDev: config read from disk successful")
    }
    if(this->readCache(cacheJson)) {
      LOG_INFO("ConfigManagerDev: cache read from disk successful")
    }

    LOG_INFO("ConfigManagerDev: getting the license")
    this->getLicense();

    // Flush to disk
    if(this->writeConfig(configJson)) {
      LOG_INFO("ConfigManagerDev: config write successful")
    } else {
      LOG_ERROR("ConfigManagerDev: config write failed")
    }
    this->writeCache(cacheJson);

    LOG_INFO("ConfigManagerDev: periodic thread finished")
    this->mutex.unlock();
    Port::threadSleep(configManagerThreadPeriodMs);
  }
}

void ConfigManager::waitInit()
{
  LOG_INFO("ConfigManagerDev: wait init started")
  // we need to at least have license information, like for example base server
  // to connect to.
  while(this->baseAdresses.empty() &&
        this->apiAddresses.empty()) {
    // busy wait
    // LOG_INFO("ConfigManagerDev: waiting")
  }
  LOG_INFO("ConfigManagerDev: wait init finished")
}

bool ConfigManager::userWhitelistAdd(const HusarnetAddress& address)
{
//  auto whitelist =
//      this->config_json["whitelist"].get<std::vector<std::string>>();
  return false;
}
void ConfigManager::triggerGetConfig()
{
  // send to periodicThread
  // some sort of IPC is here needed
}
