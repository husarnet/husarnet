// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/config_env.h"
#include "husarnet/hooks_manager.h"
#include "husarnet/ipaddress.h"

#include "etl/array.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

// ETL return limits
#ifndef MULTICAST_DESTINATIONS_LIMIT
#define MULTICAST_DESTINATIONS_LIMIT 128
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

// JSON map keys
#define USER_CONFIG_LAST_UPDATED "last_updated"
#define USER_CONFIG_WHITELIST "whitelist"

#define CACHE_LAST_UPDATED "last_updated"
#define CACHE_LICENSE "license"
#define CACHE_GET_CONFIG "get_config"

class ConfigManager {
 private:
  HooksManager* hooks_manager;
  json config_json;
  json cache_json;

  void getLicense();    // Actually do an HTTP call to TLD
  void getGetConfig();  // Actually do an HTTP call to API

  bool readConfig();  // Read from disk and save to object if possible
  bool readCache();   // Read from disk and save to object if possible

  bool writeConfig();  // If this fails we should propagate the error
  bool writeCache();   // It does not matter whether this fails

  void flush();  // Technically - just call these two writes above

 public:
  ConfigManager(HooksManager* hooks_manager, const ConfigEnv* configEnv);

  void periodicThread();  // Start as a thread - update license, flush cache to
                          // file, etc.
  void waitInit();  // Busy loop until valid enough metadata is available to
                    // function

  void triggerGetConfig();  // Trigger an async get_config pull

  // User config manipulation
  bool userWhitelistAdd(const HusarnetAddress& address);
  bool userWhitelistRm(const HusarnetAddress& address);

  // Computed value getters
  const json getStatus() const;  // All sources combined into a giant JSON to
                                 // be returned by daemon API
                                 // Status will have to also get the live
                                 // data (like is connected to base, etc) -
                                 // ideally through the HusarnetManager

  bool isPeerAllowed(const HusarnetAddress& address)
      const;  // TODO always say "yes" to all of the infra servers // This has
              // to be a high performance method
  const etl::array<HusarnetAddress, MULTICAST_DESTINATIONS_LIMIT>
  getMulticastDestinations(
      HusarnetAddress id);  // This has to be a high performance method

  // Those may change over time (license, get_config changes) so whoever
  // uses them is responsible for re-reading them periodically
  const etl::array<InternetAddress, BASE_ADDRESSES_LIMIT> getBaseAddresses()
      const;  // Note: one day this will also carry some metadata
              // about the base servers
  const etl::array<HusarnetAddress, DASHBOARD_API_ADDRESSES_LIMIT>
  getDashboardApiAddresses() const;
  const etl::array<HusarnetAddress, EVENTBUS_ADDRESSES_LIMIT>
  getEventbusAddresses() const;
};
