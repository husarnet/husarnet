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
#include "etl/string.h"
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

#ifndef EMAIL_MAX_LENGTH
#define EMAIL_MAX_LENGTH 254
#endif

#ifndef HOSTNAME_MAX_LENGTH
#define HOSTNAME_MAX_LENGTH 255
#endif

// TODO: or maybe take the value from limits.h I don't know
// On Linux it's generally up to 63 chars but Windows can go bit crazy about them afair

// JSON map keys
#define USERCONFIG_KEY_WHITELIST "whitelist"

#define CACHE_KEY_LICENSE "license"
#define CACHE_KEY_GETCONFIG "get_config"

#define GETCONFIG_KEY_PEERS "peers"
#define GETCONFIG_KEY_PEERINFO_IP "address"
#define GETCONFIG_KEY_PEERINFO_HOSTNAME "hostname"
#define GETCONFIG_KEY_PEERINFO_ALIASES "aliases"
#define GETCONFIG_KEY_IS_CLAIMED "is_claimed"
#define GETCONFIG_KEY_CLAIMINFO "claim_info"
#define GETCONFIG_KEY_CLAIMINFO_OWNER "owner"
#define GETCONFIG_KEY_CLAIMINFO_HOSTNAME "hostname"
#define GETCONFIG_KEY_FEATUREFLAGS "features"
#define GETCONFIG_KEY_FEATUREFLAGS_SYNCHOSTNAME "SyncHostname"

#define STATUS_KEY_APICONFIG "dashboard"
#define STATUS_KEY_USERCONFIG "user_config"
#define STATUS_KEY_LICENSE "license"
#define STATUS_KEY_LOCALIP "local_ip"
#define STATUS_KEY_BASECONNECTION "base_connection"
#define STATUS_KEY_BASECONNECTION_TYPE "type"
#define STATUS_KEY_BASECONNECTION_ADDRESS "address"
#define STATUS_KEY_BASECONNECTION_PORT "port"
#define STATUS_KEY_DASHBOARDCONNECTION "dashboard_connection"
#define STATUS_KEY_ENVIRONMENT "env"
#define STATUS_KEY_ENVIRONMENT_INSTANCE_FQDN "instance_fqdn"
#define STATUS_KEY_ENVIRONMENT_LOG_VERBOSITY "log_verbosity"
#define STATUS_KEY_LIVEPEERS "peers"
#define STATUS_KEY_HEALTH "health"
#define STATUS_KEY_HEALTH_SUMMARY "summary"

constexpr int configManagerPeriodInSeconds = 60 * 10;  // fresh get_config every 10 min
constexpr int refreshLicenseAfterNumPeriods = 5;       // every N get_config refreshes, refresh license too

class ConfigManager {
 private:
  HooksManager* hooksManager;
  const ConfigEnv* configEnv;

  // TODO: these might be taking up too much space for esp32 to handle
  //   we might consider moving logic for storing them to port
  json userConfigJson = json({});
  json cacheJson = json({});

  bool allowEveryone = false;  // flipped to true if control plane is disabled

  // synchronization primitives
  mutable etl::mutex mutexFast;  // protects internal sets and vectors
  mutable etl::mutex mutexSlow;  // protects json documents
  mutable etl::mutex cvMutex;    // used for condition_variable only
  std::condition_variable cv;

  etl::set<HusarnetAddress, ALLOWED_PEERS_LIMIT> allowedPeers;
  etl::set<HusarnetAddress, USER_WHITELIST_SIZE_LIMIT> userWhitelist;
  etl::vector<InternetAddress, BASE_ADDRESSES_LIMIT> baseAddresses;
  etl::vector<HusarnetAddress, DASHBOARD_API_ADDRESSES_LIMIT> apiAddresses;
  etl::vector<HusarnetAddress, EVENTBUS_ADDRESSES_LIMIT> ebAddresses;

  etl::string<EMAIL_MAX_LENGTH> claimedBy;    // empty string if not claimed
  etl::string<HOSTNAME_MAX_LENGTH> hostname;  // the one changeable from the web interface

  void getLicense();                       // HTTP call to TLD
  void storeLicense(const json& jsonDoc);  // save JSON doc
  void updateLicenseData();                // Transform JSON to internal structures

  void getGetConfig();                       // HTTP call to API
  void storeGetConfig(const json& jsonDoc);  // save JSON doc
  void updateGetConfigData();                // Transform JSON to internal structures

  bool readUserConfig();
  void storeUserConfig(const json& jsonDoc);
  void updateUserConfigData();

  bool readCache();
  void storeCache(const json& jsonDoc);

  bool writeConfig();  // If this fails we should propagate the error
  bool writeCache();   // It does not matter whether this fails

 public:
  ConfigManager(HooksManager* hooksManager, const ConfigEnv* configEnv);
  ConfigManager(const ConfigManager&) = delete;

  void periodicThread();  // Start as a thread - update license, flush cache to
                          // file, etc.
  void waitInit() const;  // Busy loop until valid enough metadata is available to
                          // function

  void triggerGetConfig();  // Trigger an async get_config pull

  // User config manipulation
  bool userWhitelistAdd(const HusarnetAddress& address);
  bool userWhitelistRm(const HusarnetAddress& address);

  // Computed value getters
  json getDataForStatus() const;  // All sources combined into a giant JSON to
                                  // be returned by daemon API
                                  // Status will have to also get the live
                                  // data (like is connected to base, etc) -
                                  // ideally through the HusarnetManager

  bool isPeerAllowed(const HusarnetAddress& address) const;

  etl::vector<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT> getMulticastDestinations(
      HusarnetAddress id);  // This has to be a high performance method

  // Those may change over time (license, get_config changes) so whoever
  // uses them is responsible for re-reading them periodically
  // Note: one day this will also carry some metadata about the base servers
  etl::vector<InternetAddress, BASE_ADDRESSES_LIMIT> getBaseAddresses() const;

  HusarnetAddress getApiAddress() const;
  HusarnetAddress getEbAddress() const;
};
