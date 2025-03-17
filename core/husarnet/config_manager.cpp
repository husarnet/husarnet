// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/config_manager.h"

#include <etl/string.h>
#include <sockets.h>

#include "husarnet/husarnet_config.h"
#include "husarnet/licensing.h"
#include "husarnet/dashboardapi/response.h"

#include "ngsocket_messages.h"

ConfigManager::ConfigManager(
    const HooksManager* hooksManager,  // TODO: currently unused
    const ConfigEnv* configEnv)
    : hooksManager(hooksManager), configEnv(configEnv)
{
  std::lock_guard lock(this->configMutex);
  if(!configEnv->getEnableControlplane()) {
    this->allowEveryone = true;
    LOG_WARNING("ConfigManagerDev: control plane is disabled, any peer will be let through")
  }
}

void ConfigManager::getLicense()
{
  auto tldAddress = this->configEnv->getTldFqdn();
  auto [statusCode, bytes] = Port::httpGet(tldAddress, "/license.json");

  if(statusCode != 200) {
    LOG_ERROR("failed to download license file (using tld address %s)", tldAddress.c_str());
    return;
  }

  auto licenseJson = json::parse(bytes, nullptr, false);
  if(!isLicenseValid(licenseJson)) {
    LOG_ERROR("license file downloaded from %s is invalid", tldAddress.c_str());
    return;
  }

  this->updateLicenseData(licenseJson);
}

// TODO: discuss: might also be renamed to updateControlPlaneData
void ConfigManager::updateLicenseData(const json& licenseJson)
{
  std::lock_guard lock(this->configMutex);
  const auto& ebServerIps = licenseJson[LICENSE_EB_SERVERS_KEY].get<std::vector<std::string>>();
  this->ebAddresses.clear();
  for(auto& ipStr : ebServerIps) {
    const auto ip = HusarnetAddress::parse(ipStr);
    this->ebAddresses.push_back(ip);
    this->allowedPeers.insert(ip);
  }

  const auto& apiServerIps = licenseJson[LICENSE_API_SERVERS_KEY].get<std::vector<std::string>>();

  this->apiAddresses.clear();
  for(auto& ipStr : apiServerIps) {
    const auto ip = HusarnetAddress::parse(ipStr);
    this->apiAddresses.push_back(ip);
    this->allowedPeers.insert(ip);
  }

  const auto baseServerIps = licenseJson[LICENSE_BASE_SERVER_ADDRESSES_KEY].get<std::vector<std::string>>();

  this->baseAddresses.clear();
  for(auto& ip : baseServerIps) {
    this->baseAddresses.push_back(InternetAddress::parse(ip));
  }
}

HusarnetAddress ConfigManager::getApiAddress() const
{
  std::lock_guard lock(this->configMutex);
  if(this->apiAddresses.empty()) {
    return {};
  }
  auto addr = this->apiAddresses[0];
  return addr;
}

HusarnetAddress ConfigManager::getEbAddress() const
{
  std::lock_guard lock(this->configMutex);
  if(this->ebAddresses.empty()) {
    return {};
  }
  auto addr = this->ebAddresses[0];
  return addr;
}

void ConfigManager::updateGetConfigData(const json& configJson)
{
  std::lock_guard lock(this->configMutex);
  LOG_INFO("ConfigManagerDev: updateGetConfigData started")

  if(configJson.contains(CONFIG_PEERS_KEY) && configJson[CONFIG_PEERS_KEY].is_array()) {
    allowedPeers.clear();
    // rebuild allowlist
    for(auto& addr : this->apiAddresses) {
      this->allowedPeers.insert(addr);
    }
    for(auto& addr : this->ebAddresses) {
      this->allowedPeers.insert(addr);
    }
    for(auto& peerInfo : configJson[CONFIG_PEERS_KEY]) {
      // unpack PeerInfo structure
      auto addr = peerInfo["address"].get<std::string>();
      LOG_INFO("ConfigManagerDev: parse %s", addr.c_str())
      this->allowedPeers.insert(HusarnetAddress::parse(addr));
      // TODO: there is also hostname and aliases, which we will need to
      // insert into hosts file, but this will happen asynchronously (probably)
      // TODO: handle set is full
    }
  }

  LOG_INFO("ConfigManagerDev: updateGetConfigData finished")
}

void ConfigManager::getGetConfig(json& configJson)
{
  auto apiResponse = dashboardapi::getConfig(this->getApiAddress());

  if(apiResponse.isSuccessful()) {
    this->updateGetConfigData(apiResponse.getPayloadJson());
    configJson = apiResponse.getPayloadJson();
  } else {
    LOG_ERROR(
        "ConfigManagerDev: API responded with error, details: %s", apiResponse.toString().c_str());
  }
}

bool ConfigManager::readConfig(json& configJson)
{
  auto contents = Port::readStorage(StorageKey::config);
  if(contents.empty()) {
    LOG_INFO("ConfigManagerDev: saved config.json is empty/nonexistent");
    return false;
  }
  auto parsedContents = json::parse(contents, nullptr, false);
  if(parsedContents.is_discarded()) {
    LOG_INFO("ConfigManagerDev: saved config.json is not a valid JSON, not reading");
    return false;
  }
  configJson = parsedContents;
  return true;
}

bool ConfigManager::readCache(json& cacheJson)
{
  auto contents = Port::readStorage(StorageKey::cache);
  if(contents.empty()) {
    LOG_INFO("ConfigManagerDev: saved cache.json is empty/nonexistent");
    return false;
  }
  auto parsedContents = json::parse(contents, nullptr, false);
  if(parsedContents.is_discarded()) {
    LOG_INFO("ConfigManagerDev: saved cache.json is not a valid JSON, not reading");
    return false;
  }
  cacheJson = parsedContents;
  return true;
}

bool ConfigManager::isPeerAllowed(const HusarnetAddress& address) const
{
  std::lock_guard lock(this->configMutex);
  if(this->allowEveryone) {
    return true;
  }
  bool isAllowed = this->allowedPeers.contains(address);
  return isAllowed;
}

etl::vector<InternetAddress, BASE_ADDRESSES_LIMIT> ConfigManager::getBaseAddresses() const
{
  std::lock_guard lock(this->configMutex);
  auto addresses = this->baseAddresses;
  return addresses;
}

etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT> ConfigManager::getMulticastDestinations(HusarnetAddress id)
{
  std::lock_guard lock(this->configMutex);
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
  // TODO: implement. The idea is to dump every possible information that is available
  // I know, we will be just keeping cache

  return json::parse("{}");
}

bool ConfigManager::writeConfig(const json& configJson)
{
  return Port::writeStorage(StorageKey::config, configJson.dump(JSON_INDENT_SPACES));
}

bool ConfigManager::writeCache(const json& cacheJson)
{
  return Port::writeStorage(StorageKey::cache, cacheJson.dump(JSON_INDENT_SPACES));
}

void ConfigManager::periodicThread()
{
  // initialization
  json configJson;
  json cacheJson;

  if(this->readConfig(configJson)) {
    LOG_INFO("ConfigManagerDev: config read from disk successful")
    this->updateGetConfigData(configJson);
  }
  if(this->readCache(cacheJson)) {
    LOG_INFO("ConfigManagerDev: cache read from disk successful")
    // TODO: determine cache structure
  }

  LOG_INFO("ConfigManagerDev: downloading the license")
  this->getLicense();  // TODO: we should periodically refresh the license too
  // TODO: add the possibility to get license from cache if no internet

  if(!this->configEnv->getEnableControlplane()) {
    LOG_INFO("ConfigManagerDev: not starting periodic thread (will not download get_config)")
    return;
  }

  while(true) {
    std::unique_lock lk(this->cvMutex);
    this->cv.wait_for(lk, std::chrono::seconds(configManagerPeriodInSeconds));
    LOG_INFO("ConfigManagerDev: periodic thread will download the config")
    this->getGetConfig(configJson);

    // Flush to disk
    if(this->writeConfig(configJson)) {
      LOG_INFO("ConfigManagerDev: config write successful")
    } else {
      LOG_ERROR("ConfigManagerDev: config write failed")
    }
    this->writeCache(cacheJson);  // best-effort

    LOG_INFO("ConfigManagerDev: periodic thread going to sleep")
  }
}

void ConfigManager::waitInit() const
{
  LOG_INFO("ConfigManagerDev: wait init started")
  // we need to at least have license information, like for example base server to connect to.
  while(this->baseAddresses.empty()) {
    // busy waiting on purpose
    // TODO: prbably needs a mutex unlock/lock too
    // maybe also reconsider changing it to condition variable wait
  }
  LOG_INFO("ConfigManagerDev: wait init finished")
}

bool ConfigManager::userWhitelistAdd(const HusarnetAddress& address)
{
  if(this->userWhitelist.contains(address)) {
    return true;
  }
  if(this->userWhitelist.full()) {
    LOG_ERROR("ConfigManagerDev: userWhitelist is full")
    return false;
  }

  this->userWhitelist.insert(address);
  this->allowedPeers.insert(address);
  return true;
}

bool ConfigManager::userWhitelistRm(const HusarnetAddress& address)
{
  if(this->userWhitelist.contains(address)) {
    this->userWhitelist.erase(address);
    this->allowedPeers.erase(address);
    return true;
  }
  return false;
}

void ConfigManager::triggerGetConfig()
{
  LOG_INFO("ConfigManagerDev: get_config ordered by control plane")
  this->cv.notify_one();
}
