// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <condition_variable>
#include <string>

#include "husarnet/config_env.h"
#include "husarnet/hooks_manager.h"
#include "husarnet/ipaddress.h"

#include "etl/mutex.h"
#include "etl/set.h"
#include "etl/vector.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

// ETL return limits
#ifndef MULTICAST_DESTINATIONS_LIMIT
#define MULTICAST_DESTINATIONS_LIMIT 128
#endif

#ifndef ALLOWED_PEERS_LIMIT
#define ALLOWED_PEERS_LIMIT 128
#endif

#ifndef BASE_ADDRESSES_LIMIT
#define BASE_ADDRESSES_LIMIT 16
#endif

#ifndef DASHBOARD_API_ADDRESSES_LIMIT
#define DASHBOARD_API_ADDRESSES_LIMIT 16
#endif

#ifndef EVENTBUS_ADDRESSES_LIMIT
#define EVENTBUS_ADDRESSES_LIMIT 16
#endif

#ifndef USER_WHITELIST_SIZE_LIMIT
#define USER_WHITELIST_SIZE_LIMIT 8
#endif

// JSON map keys
#define USER_CONFIG_LAST_UPDATED "last_updated"
#define USER_CONFIG_WHITELIST "whitelist"

#define CACHE_LAST_UPDATED "last_updated"
#define CACHE_LICENSE "license"
#define CACHE_GET_CONFIG "get_config"

#define CONFIG_ENV_KEY "env"
#define CONFIG_PEERS_KEY "peers"
#define ENV_TLD_FQDN "tldFqdn"

constexpr int configManagerPeriodInSeconds = 60 * 10;  // fresh get_config every 10 min

class ConfigManager {
 private:
  const HooksManager* hooksManager;
  const ConfigEnv* configEnv;

  // flipped to true if control plane is disabled
  bool allowEveryone = false;

  // synchronization primitives
  mutable etl::mutex configMutex;
  mutable etl::mutex cvMutex;
  std::condition_variable cv;

  etl::set<HusarnetAddress, ALLOWED_PEERS_LIMIT> allowedPeers;
  etl::set<HusarnetAddress, USER_WHITELIST_SIZE_LIMIT> userWhitelist;
  etl::vector<InternetAddress, BASE_ADDRESSES_LIMIT> baseAddresses;
  etl::vector<HusarnetAddress, DASHBOARD_API_ADDRESSES_LIMIT> apiAddresses;
  etl::vector<HusarnetAddress, EVENTBUS_ADDRESSES_LIMIT> ebAddresses;

  void getLicense();                                 // Actually do an HTTP call to TLD
  void updateLicenseData(const json& licenseJson);   // Transform JSON to internal structures
  void getGetConfig();                               // Actually do an HTTP call to API
  void updateGetConfigData(const json& configJson);  // Transform JSON to internal structures

  bool readConfig(json& jsonDoc);  // Read from disk and save to object if possible
  bool readCache(json& jsonDoc);   // Read from disk and save to object if possible

  bool writeConfig(const json& jsonDoc);  // If this fails we should propagate the error
  bool writeCache(const json& jsonDoc);   // It does not matter whether this fails

  bool isApiResponseSuccessful(json& jsonDoc) const;

 public:
  ConfigManager(const HooksManager* hooksManager, const ConfigEnv* configEnv);

  [[noreturn]] void periodicThread();  // Start as a thread - update license, flush cache to
                                       // file, etc.
  void waitInit() const;                     // Busy loop until valid enough metadata is available to
                                       // function

  void triggerGetConfig();  // Trigger an async get_config pull

  bool isApiResponseSuccessful(const json& jsonDoc) const;
  std::string apiResponseToErrorString(const json& jsonDoc) const;
  
  // User config manipulation
  bool userWhitelistAdd(const HusarnetAddress& address);
  bool userWhitelistRm(const HusarnetAddress& address);

  // Computed value getters
  const json getStatus() const;  // All sources combined into a giant JSON to
                                 // be returned by daemon API
                                 // Status will have to also get the live
                                 // data (like is connected to base, etc) -
                                 // ideally through the HusarnetManager

  bool isPeerAllowed(const HusarnetAddress& address) const;

  etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT> getMulticastDestinations(
      HusarnetAddress id);  // This has to be a high performance method

  // Those may change over time (license, get_config changes) so whoever
  // uses them is responsible for re-reading them periodically
  const etl::vector<InternetAddress, BASE_ADDRESSES_LIMIT>& getBaseAddresses()
      const;  // Note: one day this will also carry some metadata
              // about the base servers
  const etl::vector<HusarnetAddress, DASHBOARD_API_ADDRESSES_LIMIT>& getDashboardApiAddresses() const;
  const etl::vector<HusarnetAddress, EVENTBUS_ADDRESSES_LIMIT>& getEventbusAddresses() const;

  HusarnetAddress getCurrentApiAddress() const;
};
