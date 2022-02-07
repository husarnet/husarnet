#pragma once
#include "port.h"
#include "threads_port.h"

#include <unordered_set>
#include <vector>
#include "configtable.h"
#include "licensing.h"
#include "logmanager.h"
#include "ngsocket.h"
#include "websetup.h"

using HostsFileUpdateFunc =
    std::function<void(std::vector<std::pair<IpAddress, std::string>>)>;

class HusarnetManager {
  Identity* identity;        // @TODO
  NgSocket* sock;            // @TODO
  ConfigTable* configTable;  // @TODO
  LogManager* logManager;    // @TODO
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

  std::vector<DeviceId> getMulticastDestinations(DeviceId id);

  std::string getVersion();
  std::string getUserAgent();
  IpAddress getSelfAddress();
  IpAddress getCurrentBaseAddress();
  std::string getCurrentBaseProtocol();

  int getLatency(IpAddress destination);
  void cleanup();

  void updateHosts();
  std::string getWebsetupSecret();
  std::string setWebsetupSecret(std::string newSecret);
  void joinNetwork(std::string joinCode, std::string hostname);
  IpAddress resolveHostname(std::string hostname);
  std::string getSelfHostname();
  void setSelfHostname(std::string newHostname);
  bool isJoined();
  void hostTableAdd(std::string hostname, IpAddress address);
  void hostTableRm(std::string hostname);
  void whitelistEnable();
  void whitelistDisable();
  void whitelistAdd(IpAddress address);
  void whitelistRm(IpAddress address);
  std::list<IpAddress> getWhitelist();

  // Copy of methods from License class
  std::string getDashboardUrl();
  IpAddress getWebsetupAddress();
  std::list<IpAddress> getBaseServerAddresses();

  void runHusarnet();
};
