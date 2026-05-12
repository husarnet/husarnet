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
      nextLicenseDownload(std::chrono::steady_clock::now()),
      nextGetConfigUpdate(std::chrono::steady_clock::now())
{
  std::lock_guard lgFast(this->mutexFast);
  if(!configEnv->getEnableControlplane()) {
    this->allowEveryone = true;
    HLOG_WARNING("config manager: control plane is disabled, any peer will be let through");
  }
}

bool ConfigManager::fetchLicenseJson()
{
  auto tldAddress = this->configEnv->getTldFqdn();
  auto [statusCode, bytes] = Port::httpGet(tldAddress, "/license.json");

  if(statusCode != 200) {
    HLOG_ERROR("failed to download license file // {tld_address}", tldAddress);
    return false;
  }

  auto licenseJson = json::parse(bytes, nullptr, false);
  if(!isLicenseValid(licenseJson)) {
    HLOG_CRITICAL("dowloaded license file is invalid // {tld_address}", tldAddress);
    return false;
  }

  this->storeLicense(licenseJson);
  this->nextLicenseDownload = std::chrono::steady_clock::now() + licenseRefreshPeriod;
  return true;
}

void ConfigManager::storeLicense(const nlohmann::json& jsonDoc)
{
  std::lock_guard lgSlow(this->mutexSlow);
  this->cacheJson[CACHE_KEY_LICENSE] = jsonDoc;
}

void ConfigManager::updateLicenseData()
{
  std::lock_guard lgSlow(this->mutexSlow);
  std::lock_guard lgFast(this->mutexFast);
  const auto& licenseJson = this->cacheJson[CACHE_KEY_LICENSE];
  if(licenseJson.is_discarded() || licenseJson.is_null()) {
    HLOG_CRITICAL("config manager: no license information available");
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

void ConfigManager::fetchGetConfig()
{
  auto apiResponse = dashboardapi::getConfig(this->getApiAddress());
  if(apiResponse.isSuccessful()) {
    this->storeGetConfig(apiResponse.getPayloadJson());
    this->updateGetConfigData();
    this->nextGetConfigUpdate = std::chrono::steady_clock::now() + getConfigRefreshPeriod;
  } else {
    HLOG_ERROR("config manager: API responded with error // {response}", apiResponse.toString());
  }
}

void ConfigManager::updateGetConfigData()
{
  std::lock_guard lgSlow(this->mutexSlow);
  std::lock_guard lgFast(this->mutexFast);
  HLOG_INFO("config manager: updateGetConfigData started");
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

    if(!previousOwner.empty() && this->claimedBy.empty()) {
      HLOG_INFO("device was unclaimed");
      this->hooksManager->scheduleHook(HookType::unclaimed);
    }

    std::map<std::string, HusarnetAddress> hostsEntries;
    for(auto& peerInfo : latestConfig[GETCONFIG_KEY_PEERS]) {
      // unpack PeerInfo structure
      auto addrStr = peerInfo[GETCONFIG_KEY_PEERINFO_IP].get<std::string>();
      auto peerHostname = peerInfo[GETCONFIG_KEY_PEERINFO_HOSTNAME].get<std::string>();
      auto aliases = peerInfo[GETCONFIG_KEY_PEERINFO_ALIASES].get<std::vector<std::string>>();

      HLOG_DEBUG("parsing address from get_config // {address}", addrStr);
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

  HLOG_INFO("updateGetConfigData finished");
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
    HLOG_INFO("saved config.json is empty/nonexistent");
    contents = "{}";  // avoid parsing empty string, which results in discarded json
  }
  auto parsedContents = json::parse(contents, nullptr, false);
  if(parsedContents.is_discarded()) {
    HLOG_INFO("saved config.json is not a valid JSON, not reading");
    this->storeUserConfig(json({}));  // reset to empty config
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
        HLOG_WARNING("user whitelist contains invalid Husarnet address // {address}", entry);
      }
    }
  }
}

bool ConfigManager::readCache()
{
  auto contents = Port::readStorage(StorageKey::cache);
  if(contents.empty()) {
    HLOG_INFO("config manager: saved cache.json is empty/nonexistent");
    return false;
  }
  auto parsedContents = json::parse(contents, nullptr, false);
  if(parsedContents.is_discarded()) {
    HLOG_ERROR("config manager: saved cache.json is not a valid JSON");
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
      {STATUS_KEY_ENVIRONMENT_LOG_VERBOSITY, this->configEnv->getLogVerbosityString()},
      {STATUS_KEY_ENVIRONMENT_HOOKS_ENABLED, this->configEnv->getEnableHooks()},
      {STATUS_KEY_ENVIRONMENT_DAEMON_API_HOST, this->configEnv->getDaemonApiHost().toString()},
      {STATUS_KEY_ENVIRONMENT_DAEMON_API_PORT, this->configEnv->getDaemonApiPort()}};

  return combined;
}

bool ConfigManager::writeUserConfig()
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
    HLOG_INFO("cache.json read from disk successful");
  }
  if(this->readUserConfig()) {
    HLOG_INFO("config.json read from disk successful");
    this->updateUserConfigData();
  }

  if(!this->fetchLicenseJson()) {
    HLOG_INFO("license download failed, will use cached data if available");
  }
  this->updateLicenseData();
  this->writeCache();

  while(true) {
    std::unique_lock lk(this->cvMutex);
    this->cv.wait_for(lk, std::chrono::milliseconds(periodicThreadIntervalMs));

    TimePoint now = std::chrono::steady_clock::now();
    bool shouldWriteCache = false;

    if(now >= this->nextLicenseDownload) {
      HLOG_DEBUG("config manager: periodic thread: will redownload the license");
      this->fetchLicenseJson();
      shouldWriteCache = true;
    }

    if(this->configEnv->getEnableControlplane() && now >= this->nextGetConfigUpdate) {
      HLOG_DEBUG("config manager: periodic thread: will request the config from the control plane");
      this->fetchGetConfig();
      shouldWriteCache = true;
    }

    if(shouldWriteCache) {
      this->writeCache();
    }
  }
}

void ConfigManager::waitInit() const
{
  HLOG_INFO("config manager: wait init started");
  // we need to at least have license information, like for example base server to connect to.
  // wait until license is downloaded and base addresses are known
  while(this->baseAddresses.empty()) {
    Port::threadSleep(20);
  }
  HLOG_INFO("config manager: wait init finished");
}

bool ConfigManager::userWhitelistAdd(const HusarnetAddress& address)
{
  std::lock_guard lock(this->mutexFast);
  if(this->userWhitelist.contains(address)) {
    HLOG_WARNING("config manager: IP is already whitelisted");
    return false;
  }
  if(this->userWhitelist.full()) {
    HLOG_ERROR("config manager: userWhitelist is full");
    return false;
  }

  this->userWhitelist.insert(address);

  // Flush to disk
  if(this->writeUserConfig()) {
    HLOG_DEBUG("user config write successful");
  } else {
    HLOG_ERROR("user config write failed");
  }

  return true;
}

bool ConfigManager::userWhitelistRm(const HusarnetAddress& address)
{
  std::lock_guard lock(this->mutexFast);
  if(this->userWhitelist.contains(address)) {
    this->userWhitelist.erase(address);
    return true;
  }
  HLOG_WARNING("config manager: IP is not on the whitelist");
  return false;
}

void ConfigManager::triggerGetConfig()
{
  HLOG_INFO("config manager: get_config ordered by control plane, resetting timer");
  this->nextGetConfigUpdate = std::chrono::steady_clock::now();
  this->cv.notify_one();
}

int ConfigManager::getWorkerQueueSize() const
{
  return this->configEnv->getWorkerQueueSize();
}
