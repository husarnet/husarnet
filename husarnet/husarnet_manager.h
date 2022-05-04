// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "ports/port.h"

#include <unordered_set>
#include <vector>
#include "husarnet/config_storage.h"
#include "husarnet/ipaddress.h"
#include "husarnet/licensing.h"
#include "husarnet/logmanager.h"
#include "husarnet/ngsocket.h"
#include "husarnet/websetup.h"

using HostsFileUpdateFunc =
    std::function<void(std::vector<std::pair<IpAddress, std::string>>)>;

class HusarnetManager {
  Identity identity;
  NgSocket* ngsocket;
  ConfigStorage* configStorage;
  LogManager* logManager;
  WebsetupConnection* websetup;
  License* license;
  std::vector<std::thread*> threadpool;

  bool stage1Started = false;
  bool stage2Started = false;
  bool stage3Started = false;

  std::string
  configGet(std::string networkId, std::string key, std::string defaultValue);
  std::string
  configSet(std::string networkId, std::string key, std::string value);

  void getLicense();
  void getIdentity();
  void startNGSocket();
  void startWebsetup();
  void startHTTPServer();

 public:
  HusarnetManager();
  HusarnetManager(const HusarnetManager&) = delete;

  LogManager& getLogManager();

  std::string getVersion();
  std::string getUserAgent();

  IpAddress getSelfAddress();
  std::string getSelfHostname();
  void setSelfHostname(std::string newHostname);

  void updateHosts();
  IpAddress resolveHostname(std::string hostname);

  IpAddress getCurrentBaseAddress();
  std::string getCurrentBaseProtocol();

  std::string getWebsetupSecret();
  std::string setWebsetupSecret(std::string newSecret);

  void joinNetwork(std::string joinCode, std::string hostname);
  bool isJoined();

  void hostTableAdd(std::string hostname, IpAddress address);
  void hostTableRm(std::string hostname);

  void whitelistAdd(IpAddress address);
  void whitelistRm(IpAddress address);
  std::list<IpAddress> getWhitelist();

  bool isWhitelistEnabled();
  void whitelistEnable();
  void whitelistDisable();

  bool isHostAllowed(IpAddress id);

  // TODO think about exposing license object in the same way logmanager is
  // Copy of methods from License class
  std::string getDashboardUrl();
  IpAddress getWebsetupAddress();
  std::vector<IpAddress> getBaseServerAddresses();

  std::vector<DeviceId> getMulticastDestinations(DeviceId id);
  int getLatency(IpAddress destination);

  void cleanup();

  void stage1();  // Start privileged interface and config storage so user code
                  // (on platforms like ESP32) can modify settings
  void stage2();  // Read identity, obtain license, etc. Changing user settings
                  // is not allowed after this step
  void stage3();  // Actually connect to the network
  void runHusarnet();
};
