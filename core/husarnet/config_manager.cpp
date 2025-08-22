// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/config_manager.h"

#include <etl/string.h>
#include <sockets.h>

#include "husarnet/dashboardapi/response.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/licensing.h"

#include "ngsocket_messages.h"

ConfigManager::ConfigManager(HooksManager* hooksManager, const ConfigEnv* configEnv, HusarnetAddress ourIp)
    : hooksManager(hooksManager),
      configEnv(configEnv),
      ourIp(ourIp),
  nextLicenseUpdate(std::chrono::steady_clock::now()),
  nextGetConfigUpdate(std::chrono::steady_clock::now())
{
  std::lock_guard lgFast(this->mutexFast);
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

  this->storeLicense(licenseJson);
  this->updateLicenseData();
  this->nextLicenseUpdate = std::chrono::steady_clock::now() + licenseRefreshPeriod;
}

void ConfigManager::storeLicense(const nlohmann::json& jsonDoc)
{
  std::lock_guard lgSlow(this->mutexSlow);
  this->cacheJson[CACHE_KEY_LICENSE] = jsonDoc;
}

// TODO: discuss: might also be renamed to updateControlPlaneData
void ConfigManager::updateLicenseData()
{
  std::lock_guard lgSlow(this->mutexSlow);
  std::lock_guard lgFast(this->mutexFast);
  const auto& licenseJson = this->cacheJson[CACHE_KEY_LICENSE];
  if(licenseJson.is_discarded() || licenseJson.is_null()) {
    LOG_ERROR("ConfigManagerDev: no license information available");
    return;
  }

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
  std::lock_guard lgFast(this->mutexFast);
  if(this->apiAddresses.empty()) {
    return {};
  }
  auto addr = this->apiAddresses[0];
  return addr;
}

HusarnetAddress ConfigManager::getEbAddress() const
{
  std::lock_guard lgFast(this->mutexFast);
  if(this->ebAddresses.empty()) {
    return {};
  }
  auto addr = this->ebAddresses[0];
  return addr;
}

void ConfigManager::getGetConfig()
{
  auto apiResponse = dashboardapi::getConfig(this->getApiAddress());
  if(apiResponse.isSuccessful()) {
    this->storeGetConfig(apiResponse.getPayloadJson());
    this->updateGetConfigData();
    this->nextGetConfigUpdate = std::chrono::steady_clock::now() + getConfigRefreshPeriod;
  } else {
    LOG_ERROR("ConfigManagerDev: API responded with error, details: %s", apiResponse.toString().c_str());
  }
}

void ConfigManager::updateGetConfigData()
{
  std::lock_guard lgSlow(this->mutexSlow);
  std::lock_guard lgFast(this->mutexFast);
  LOG_INFO("ConfigManagerDev: updateGetConfigData started")
  const auto& latestConfig = this->cacheJson[CACHE_KEY_GETCONFIG];

  if(latestConfig.contains(GETCONFIG_KEY_PEERS) && latestConfig[GETCONFIG_KEY_PEERS].is_array()) {
    this->allowedPeers.clear();

    etl::string<EMAIL_MAX_LENGTH> previousOwner = this->claimedBy;
    // upack ClaimInfo
    this->claimed = latestConfig[GETCONFIG_KEY_IS_CLAIMED].get<bool>();
    if(this->claimed) {
      auto claimInfo = latestConfig[GETCONFIG_KEY_CLAIMINFO];
      auto ownerStr = claimInfo[GETCONFIG_KEY_CLAIMINFO_OWNER].get<std::string>();
      auto hostnameStr = claimInfo[GETCONFIG_KEY_CLAIMINFO_HOSTNAME].get<std::string>();
      this->claimedBy = etl::string<EMAIL_MAX_LENGTH>(ownerStr.c_str());
      this->hostname = etl::string<HOSTNAME_MAX_LENGTH>(hostnameStr.c_str());

      auto featureFlags = latestConfig[GETCONFIG_KEY_FEATUREFLAGS];
      // legacy sync hostname feature
      auto shouldSyncHostname = featureFlags[GETCONFIG_KEY_FEATUREFLAGS_SYNCHOSTNAME].get<bool>();
      if(shouldSyncHostname) {
        Port::setSelfHostname(hostnameStr);
      }
    } else {
      this->claimedBy = "";
    }

    if(previousOwner.empty() && !this->claimedBy.empty()) {
      this->hooksManager->scheduleHook(HookType::claimed);
    }
    // TODO: add unclaimed hook

    std::map<std::string, HusarnetAddress> hostsEntries;
    for(auto& peerInfo : latestConfig[GETCONFIG_KEY_PEERS]) {
      // unpack PeerInfo structure
      auto addrStr = peerInfo[GETCONFIG_KEY_PEERINFO_IP].get<std::string>();
      auto peerHostname = peerInfo[GETCONFIG_KEY_PEERINFO_HOSTNAME].get<std::string>();
      auto aliases = peerInfo[GETCONFIG_KEY_PEERINFO_ALIASES].get<std::vector<std::string>>();

      LOG_INFO("ConfigManagerDev: parse %s", addrStr.c_str())
      auto addr = HusarnetAddress::parse(addrStr);
      this->allowedPeers.insert(addr);  // TODO: handle set is full
      hostsEntries.insert({peerHostname, addr});
      for(auto& alias : aliases) {
        hostsEntries.insert({alias, addr});
      }
      // TODO it would be best if the lock was freed before updateHostsFile
      // but that's for later
    }
    // add also our own address as husarnet-local
    hostsEntries.insert({"husarnet-local", this->ourIp});
    Port::updateHostsFile(hostsEntries);
  }

  LOG_INFO("ConfigManagerDev: updateGetConfigData finished")
}

void ConfigManager::storeGetConfig(const json& jsonDoc)
{
  std::lock_guard lgSlow(this->mutexSlow);
  this->cacheJson[CACHE_KEY_GETCONFIG] = jsonDoc;
}

bool ConfigManager::readUserConfig()
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
  this->storeUserConfig(parsedContents);
  return true;
}

void ConfigManager::storeUserConfig(const json& jsonDoc)
{
  std::lock_guard lgSlow(this->mutexSlow);
  if(jsonDoc.contains(USERCONFIG_KEY_WHITELIST) && jsonDoc[USERCONFIG_KEY_WHITELIST].is_array()) {
    this->userConfigJson[USERCONFIG_KEY_WHITELIST] = jsonDoc[USERCONFIG_KEY_WHITELIST];
  }
}

void ConfigManager::updateUserConfigData()
{
  std::lock_guard lgSlow(this->mutexSlow);
  std::lock_guard lgFast(this->mutexFast);
  if(this->userConfigJson.contains(USERCONFIG_KEY_WHITELIST) &&
     this->userConfigJson[USERCONFIG_KEY_WHITELIST].is_array()) {
    this->userWhitelist.clear();
    auto entries = this->userConfigJson[USERCONFIG_KEY_WHITELIST].get<std::vector<std::string>>();
    for(auto& entry : entries) {
      auto addr = HusarnetAddress::parse(entry);
      if(addr.isFC94()) {
        this->userWhitelist.insert(addr);
      } else {
        LOG_WARNING("user whitelist contains invalid Husarnet address %s", entry.c_str());
      }
    }
  }
}

bool ConfigManager::readCache()
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

  this->storeCache(parsedContents);
  return true;
}

void ConfigManager::storeCache(const nlohmann::json& jsonDoc)
{
  std::lock_guard lgSlow(this->mutexSlow);
  if(jsonDoc.contains(CACHE_KEY_GETCONFIG) && jsonDoc[CACHE_KEY_GETCONFIG].is_object()) {
    this->cacheJson[CACHE_KEY_GETCONFIG] = jsonDoc[CACHE_KEY_GETCONFIG];
  }
  if(jsonDoc.contains(CACHE_KEY_LICENSE) && jsonDoc[CACHE_KEY_LICENSE].is_object()) {
    this->cacheJson[CACHE_KEY_LICENSE] = jsonDoc[CACHE_KEY_LICENSE];
  }
}

bool ConfigManager::isPeerAllowed(const HusarnetAddress& address) const
{
  std::lock_guard lgFast(this->mutexFast);
  if(this->allowEveryone) {
    return true;
  }
  for(auto& apiAddr : this->apiAddresses) {
    if(apiAddr == address) {
      return true;
    }
  }
  for(auto& ebAddr : this->ebAddresses) {
    if(ebAddr == address) {
      return true;
    }
  }
  if(this->userWhitelist.contains(address)) {
    return true;
  }
  return this->allowedPeers.contains(address);
}

bool ConfigManager::isClaimed() const
{
  std::lock_guard lgFast(this->mutexFast);
  return this->claimed;
}

etl::vector<InternetAddress, BASE_ADDRESSES_LIMIT> ConfigManager::getBaseAddresses() const
{
  std::lock_guard lgFast(this->mutexFast);
  return this->baseAddresses;
}

etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT> ConfigManager::getMulticastDestinations(HusarnetAddress id)
{
  std::lock_guard lgFast(this->mutexFast);
  // TODO: figure out if this check was even relevant
  //  if(!id == deviceIdFromIpAddress(multicastDestination)) {
  //    return {};
  //  }
  auto result = etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT>();
  for(auto& peer : this->allowedPeers) {
    result.push_back(peer);
  }
  return result;
}

json ConfigManager::getDataForStatus() const
{
  std::lock_guard lgSlow(this->mutexSlow);
  json combined;
  combined[STATUS_KEY_USERCONFIG] = this->userConfigJson;

  if(this->cacheJson.contains(CACHE_KEY_GETCONFIG)) {
    combined[STATUS_KEY_APICONFIG] = this->cacheJson[CACHE_KEY_GETCONFIG];
  } else {
    combined[STATUS_KEY_APICONFIG] = json({});
  }

  if(this->cacheJson.contains(CACHE_KEY_LICENSE)) {
    combined[STATUS_KEY_LICENSE] = this->cacheJson[CACHE_KEY_LICENSE];
  } else {
    combined[STATUS_KEY_LICENSE] = json({});
  }

  combined[STATUS_KEY_ENVIRONMENT] = {
      {STATUS_KEY_ENVIRONMENT_INSTANCE_FQDN, this->configEnv->getTldFqdn()},
      {STATUS_KEY_ENVIRONMENT_LOG_VERBOSITY, this->configEnv->getLogVerbosity()}};

  return combined;
}

bool ConfigManager::writeConfig()
{
  std::lock_guard lgSlow(this->mutexSlow);
  return Port::writeStorage(StorageKey::config, this->userConfigJson.dump(JSON_INDENT_SPACES));
}

bool ConfigManager::writeCache()
{
  std::lock_guard lgSlow(this->mutexSlow);
  return Port::writeStorage(StorageKey::cache, this->cacheJson.dump(JSON_INDENT_SPACES));
}

void ConfigManager::periodicThread()
{
  // start with disk reads and slurp json docs into ram
  if(this->readCache()) {
    LOG_INFO("ConfigManagerDev: cache read from disk successful")
  }
  if(this->readUserConfig()) {
    LOG_INFO("ConfigManagerDev: user config read from disk successful")
    updateUserConfigData();
  }

  this->getLicense();

  while(true) {
    std::unique_lock lk(this->cvMutex);
    this->cv.wait_for(lk, std::chrono::milliseconds(periodicThreadIntervalMs));

    TimePoint now = std::chrono::steady_clock::now();

    if(now >= this->nextLicenseUpdate) {
      LOG_DEBUG("ConfigManagerDev: periodic thread: will redownload the license")
      this->getLicense();
    }

    if(this->configEnv->getEnableControlplane() && now >= this->nextGetConfigUpdate) {
      LOG_DEBUG("ConfigManagerDev: periodic thread: will request the config from the control plane");
      this->getGetConfig();

      // Flush to disk
      if(this->writeConfig()) {
        LOG_DEBUG("ConfigManagerDev: config write successful")
      } else {
        LOG_ERROR("ConfigManagerDev: config write failed")
      }

      this->writeCache();  // best-effort
    }
  }
}

void ConfigManager::waitInit() const
{
  LOG_INFO("ConfigManagerDev: wait init started")
  // we need to at least have license information, like for example base server to connect to.
  // wait until license is downloaded and base addresses are known
  while(this->baseAddresses.empty()) {
    Port::threadSleep(20);
  }
  LOG_INFO("ConfigManagerDev: wait init finished")
}

bool ConfigManager::userWhitelistAdd(const HusarnetAddress& address)
{
  std::lock_guard lock(this->mutexFast);
  if(this->userWhitelist.contains(address)) {
    LOG_ERROR("ConfigManagerDev: IP is already whitelisted")
    return false;
  }
  if(this->userWhitelist.full()) {
    LOG_ERROR("ConfigManagerDev: userWhitelist is full")
    return false;
  }

  this->userWhitelist.insert(address);
  return true;
}

bool ConfigManager::userWhitelistRm(const HusarnetAddress& address)
{
  std::lock_guard lock(this->mutexFast);
  if(this->userWhitelist.contains(address)) {
    this->userWhitelist.erase(address);
    return true;
  }
  LOG_ERROR("ConfigManagerDev: IP is not on the whitelist")
  return false;
}

void ConfigManager::triggerGetConfig()
{
  LOG_INFO("ConfigManagerDev: get_config ordered by control plane, resetting timer")
  this->nextGetConfigUpdate = std::chrono::steady_clock::now();
  this->cv.notify_one();
}
