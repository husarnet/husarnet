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
    const HooksManager* hooksManager,  // TODO: currently unused
    const ConfigEnv* configEnv)
    : hooksManager(hooksManager), configEnv(configEnv)
{
  this->configMutex.lock();
  if(!configEnv->getEnableControlplane()) {
    allowEveryone = true;
    LOG_WARNING("ConfigManagerDev: control plane is disabled, any peer will be let through")
  }
  this->configMutex.unlock();
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
    LOG_CRITICAL("license file downloaded from %s is invalid", tldAddress.c_str());
    return;
  }

  this->updateLicenseData(licenseJson);
}

// TODO: discuss: might also be renamed to updateControlPlaneData
void ConfigManager::updateLicenseData(const json& licenseJson)
{
  this->configMutex.lock();
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
  this->configMutex.unlock();
}

HusarnetAddress ConfigManager::getApiAddress() const
{
  this->configMutex.lock();
  if(this->apiAddresses.empty()) {
    return {};
  }
  auto addr = this->apiAddresses[0];
  this->configMutex.unlock();
  return addr;
}

HusarnetAddress ConfigManager::getEbAddress() const
{
  this->configMutex.lock();
  if(this->ebAddresses.empty()) {
    return {};
  }
  auto addr = this->ebAddresses[0];
  this->configMutex.unlock();
  return addr;
}

InternetAddress ConfigManager::getBaseAddress() const
{
  this->configMutex.lock();
  if(this->baseAddresses.empty()) {
    return {};
  }
  auto addr = this->baseAddresses[0];
  this->configMutex.unlock();
  return addr;
}

void ConfigManager::updateGetConfigData(const json& configJson)
{
  this->configMutex.lock();
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
    }
  }

  LOG_INFO("ConfigManagerDev: updateGetConfigData finished")
  this->configMutex.unlock();
}

void ConfigManager::getGetConfig(json& configJson)
{
  auto apiAddress = this->getApiAddress();
  auto [statusCode, bytes] = Port::httpGet(apiAddress.toString(), "/device/get_config");

  if(statusCode != 200) {
    LOG_ERROR(
        "ConfigManagerDev: failed to obtain latest config (http request to %s was unsuccessful)",
        apiAddress.toString().c_str());
    return;
  }

  LOG_INFO("ConfigManagerDev: config: %s ", bytes.c_str());

  auto apiResponse = json::parse(bytes);
  if(this->isApiResponseSuccessful(apiResponse)) {
    this->updateGetConfigData(apiResponse["payload"]);
  } else {
    LOG_ERROR(
        "ConfigManagerDev: API responded with error, details: %s", this->apiResponseToErrorString(apiResponse).c_str());
  }
}

// TODO: move those to own/class module
bool ConfigManager::isApiResponseSuccessful(const json& jsonDoc) const
{
  return jsonDoc["type"].get<std::string>() == "success";
}

std::string ConfigManager::apiResponseToErrorString(const json& jsonDoc) const
{
  std::string result{};
  if(jsonDoc["type"] == "user_error") {
    result += "invalid request: ";
  } else if(jsonDoc["type"] == "server_error") {
    result += "server error: ";
  } else {
    result += "unknown api error: ";
  }

  auto errors = jsonDoc["errors"].get<std::vector<std::string>>();
  for(auto& err : errors) {
    result += err;
  }

  return result;
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
  this->configMutex.lock();
  if(this->allowEveryone) {
    this->configMutex.unlock();
    return true;
  }
  bool isAllowed = this->allowedPeers.contains(address);
  this->configMutex.unlock();
  return isAllowed;
}

etl::vector<InternetAddress, BASE_ADDRESSES_LIMIT> ConfigManager::getBaseAddresses() const
{
  return this->baseAddresses;
}

etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT> ConfigManager::getMulticastDestinations(HusarnetAddress id)
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
    std::unique_lock<etl::mutex> lk(this->cvMutex);
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
  while(this->baseAddresses.empty() && this->apiAddresses.empty()) {
    // busy waiting on purpose
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
