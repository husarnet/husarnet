#pragma once
#include "ports/port.h"

#include <unordered_set>
#include <vector>
#include "config_storage.h"
#include "licensing.h"
#include "logmanager.h"
#include "ngsocket.h"
#include "websetup.h"

using HostsFileUpdateFunc =
    std::function<void(std::vector<std::pair<IpAddress, std::string>>)>;

class HusarnetManager {
  Identity* identity;  // @TODO
  NgSocket* sock;      // @TODO
  ConfigStorage* configStorage;
  LogManager* logManager;  // @TODO
  WebsetupConnection* websetup;
  License* license;

  std::string configGet(std::string networkId,
                        std::string key,
                        std::string defaultValue);
  std::string configSet(std::string networkId,
                        std::string key,
                        std::string value);

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

  void whitelistEnable();
  void whitelistDisable();

  // @TODO think about exposing license object in the same way logmanager is
  // Copy of methods from License class
  std::string getDashboardUrl();
  IpAddress getWebsetupAddress();
  std::vector<IpAddress> getBaseServerAddresses();

  std::vector<DeviceId> getMulticastDestinations(DeviceId id);
  int getLatency(IpAddress destination);

  void cleanup();

  void runHusarnet();
};
