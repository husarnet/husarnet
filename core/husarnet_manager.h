// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "husarnet/config_storage.h"
#include "husarnet/device_id.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/licensing.h"
#include "husarnet/ngsocket.h"
#include "husarnet/peer_container.h"
#include "husarnet/security_layer.h"
#include "husarnet/websetup.h"

#include "ports/port.h"

class SecurityLayer;
class ConfigStorage;
class License;
class NgSocket;
class PeerContainer;
class PeerFlags;
class WebsetupConnection;

using HostsFileUpdateFunc =
    std::function<void(std::vector<std::pair<IpAddress, std::string>>)>;

class HusarnetManager {
 private:
  Identity identity;
  PeerFlags* selfFlags;
  NgSocket* ngsocket;
  SecurityLayer* securityLayer;
  ConfigStorage* configStorage;
  PeerContainer* peerContainer;
  WebsetupConnection* websetup;
  License* license;
  std::vector<std::thread*> threadpool;

  bool stage1Started = false;
  bool stage2Started = false;
  bool stage3Started = false;
  bool dirty = false;

  void getLicenseStage();
  void getIdentityStage();
  void startNetworkingStack();
  void startWebsetup();
  void startHTTPServer();

 public:
  HusarnetManager();
  HusarnetManager(const HusarnetManager&) = delete;

  ConfigStorage& getConfigStorage();
  void setConfigStorage(ConfigStorage* cs);
  PeerContainer* getPeerContainer();

  std::string getVersion();
  std::string getUserAgent();

  void readLegacyConfig();

  Identity* getIdentity();
  IpAddress getSelfAddress();
  PeerFlags* getSelfFlags();

  std::string getSelfHostname();
  bool setSelfHostname(std::string newHostname);

  void updateHosts();
  IpAddress resolveHostname(std::string hostname);

  InetAddress getCurrentBaseAddress();
  std::string getCurrentBaseProtocol();

  bool isConnectedToBase();
  bool isConnectedToWebsetup();

  // Husarnet daemon is "dirty" when a restart-requiring change in the
  // configuration was made, but the restart was not yet performed.
  bool isDirty();
  void setDirty();

  std::string getWebsetupSecret();
  std::string setWebsetupSecret(std::string newSecret);

  void joinNetwork(std::string joinCode, std::string hostname = "");
  bool isJoined();

  void changeServer(std::string domain);

  void hostTableAdd(std::string hostname, IpAddress address);
  void hostTableRm(std::string hostname);

  void whitelistAdd(IpAddress address);
  void whitelistRm(IpAddress address);
  std::list<IpAddress> getWhitelist();

  bool isWhitelistEnabled();
  void whitelistEnable();
  void whitelistDisable();

  bool isPeerAddressAllowed(IpAddress id);
  bool isRealAddressAllowed(InetAddress addr);

  int getApiPort();
  std::string getApiSecret();
  std::string rotateApiSecret();

  // Copy of methods from License class
  std::string getDashboardFqdn();
  IpAddress getWebsetupAddress();
  std::vector<IpAddress> getBaseServerAddresses();

  NgSocket* getNGSocket();
  SecurityLayer* getSecurityLayer();
  std::string getInterfaceName();
  void setInterfaceName(std::string name);
  std::vector<DeviceId> getMulticastDestinations(DeviceId id);
  int getLatency(DeviceId destination);

  void cleanup();

  void stage1();  // Start privileged interface and config storage so user code
                  // (on platforms like ESP32) can modify settings
  void stage2();  // Read identity, obtain license, etc. Changing user settings
                  // is not allowed after this step
  void stage3();  // Actually connect to the network
  void runHusarnet();
};
