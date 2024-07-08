// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "husarnet/config_storage.h"
#include "husarnet/device_id.h"
#include "husarnet/hooks_manager.h"
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
class HooksManagerInterface;

using HostsFileUpdateFunc =
    std::function<void(std::vector<std::pair<IpAddress, std::string>>)>;

class HusarnetManager {
 private:
  Identity identity;

  License* license = nullptr;

  PeerFlags* selfFlags = nullptr;

  NgSocket* ngsocket = nullptr;
  SecurityLayer* securityLayer = nullptr;
  ConfigStorage* configStorage = nullptr;
  PeerContainer* peerContainer = nullptr;
  TunTap* tunTap = nullptr;

  WebsetupConnection* websetup = nullptr;
  HooksManagerInterface* hooksManager = nullptr;

  std::vector<std::thread*> threadpool;

  bool dirty = false;

  bool dummyMode = false;

 public:
  HusarnetManager();
  HusarnetManager(const HusarnetManager&) = delete;
  ~HusarnetManager();

  void enterDummyMode();

  ConfigStorage& getConfigStorage();
  void setConfigStorage(ConfigStorage* cs);
  PeerContainer* getPeerContainer();
  TunTap* getTunTap();

  std::string getVersion();
  std::string getUserAgent();

  Identity* getIdentity();
  IpAddress getSelfAddress();
  PeerFlags* getSelfFlags();
  HooksManagerInterface* getHooksManager();

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

  bool changeServer(std::string domain);

  void hostTableAdd(std::string hostname, IpAddress address);
  void hostTableRm(std::string hostname);

  void whitelistAdd(IpAddress address);
  void whitelistRm(IpAddress address);
  std::list<IpAddress> getWhitelist();

  bool isWhitelistEnabled();
  void whitelistEnable();
  void whitelistDisable();

  bool areHooksEnabled();
  void hooksEnable();
  void hooksDisable();

  bool isPeerAddressAllowed(IpAddress id);
  bool isRealAddressAllowed(InetAddress addr);

  LogLevel getVerbosity();
  void setVerbosity(LogLevel level);

  int getDaemonApiPort();
  IpAddress getDaemonApiAddress();
  std::string getDaemonApiInterface();
  IpAddress getDaemonApiInterfaceAddress();
  std::string getDaemonApiSecret();
  std::string rotateDaemonApiSecret();
  // Copy of methods from License class
  std::string getDashboardFqdn();
  IpAddress getWebsetupAddress();
  std::vector<IpAddress> getBaseServerAddresses();
  std::vector<IpAddress> getDashboardApiAddresses();

  NgSocket* getNGSocket();
  SecurityLayer* getSecurityLayer();
  std::string getInterfaceName();
  void setInterfaceName(std::string name);
  std::vector<DeviceId> getMulticastDestinations(DeviceId id);
  int getLatency(DeviceId destination);

  void cleanup();

  void prepareHusarnet();  // Initialize most of the necessary dependencies but
                           // do not start them yet. You should now be able to
                           // modify configuration
  void runHusarnet();
};
