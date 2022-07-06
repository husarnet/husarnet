// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/husarnet_manager.h"

#include <map>
#include <vector>

#include "husarnet/ports/port.h"
#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/privileged_interface.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/compression_layer.h"
#include "husarnet/config_storage.h"
#include "husarnet/device_id.h"
#include "husarnet/gil.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/licensing.h"
#include "husarnet/logmanager.h"
#include "husarnet/multicast_layer.h"
#include "husarnet/ngsocket.h"
#include "husarnet/peer_container.h"
#include "husarnet/peer_flags.h"
#include "husarnet/security_layer.h"
#include "husarnet/util.h"
#include "husarnet/websetup.h"

#ifdef HTTP_CONTROL_API
#include "husarnet/api_server/server.h"
#endif

LogManager& HusarnetManager::getLogManager()
{
  return *logManager;
}

ConfigStorage& HusarnetManager::getConfigStorage()
{
  return *configStorage;
}

PeerContainer* HusarnetManager::getPeerContainer()
{
  return peerContainer;
}

std::string HusarnetManager::getVersion()
{
  return HUSARNET_VERSION;
}

std::string HusarnetManager::getUserAgent()
{
  return std::string("Husarnet,") + PORT_NAME + "," + HUSARNET_VERSION;
}

Identity* HusarnetManager::getIdentity()
{
  return &identity;
}

IpAddress HusarnetManager::getSelfAddress()
{
  return identity.getIpAddress();
}

PeerFlags* HusarnetManager::getSelfFlags()
{
  return selfFlags;
}

std::string HusarnetManager::getSelfHostname()
{
  return Privileged::getSelfHostname();
}

bool HusarnetManager::setSelfHostname(std::string newHostname)
{
  return Privileged::setSelfHostname(newHostname);
}

void HusarnetManager::updateHosts()
{
  Privileged::updateHostsFile(configStorage->getHostTable());
}

IpAddress HusarnetManager::resolveHostname(std::string hostname)
{
  auto hostTable = configStorage->getHostTable();

  if(hostTable.find(hostname) == hostTable.end()) {
    return IpAddress{};
  }

  return hostTable[hostname];
}

InetAddress HusarnetManager::getCurrentBaseAddress()
{
  return ngsocket->getCurrentBaseAddress();
}

std::string HusarnetManager::getCurrentBaseProtocol()
{
  return ngsocket->getCurrentBaseConnectionType()._to_string();
}

bool HusarnetManager::isConnectedToBase()
{
  return ngsocket->getCurrentBaseConnectionType() !=
         better_enums_data_BaseConnectionType::NONE;
}

bool HusarnetManager::isConnectedToWebsetup()
{
  return websetup->getLastContact() != 0;
}

std::string HusarnetManager::getWebsetupSecret()
{
  return configStorage->getInternalSetting(InternalSetting::websetupSecret);
}

std::string HusarnetManager::setWebsetupSecret(std::string newSecret)
{
  configStorage->setInternalSetting(InternalSetting::websetupSecret, newSecret);
  return newSecret;
}

static std::string parseJoinCode(std::string joinCode)
{
  int slash = joinCode.find("/");
  if(slash == -1) {
    return joinCode;
  }

  return joinCode.substr(slash + 1);
}

void HusarnetManager::joinNetwork(std::string joinCode, std::string newHostname)
{
  whitelistAdd(getWebsetupAddress());

  std::string dashboardHostname;
  if(newHostname.empty()) {
    dashboardHostname = Privileged::getSelfHostname();
  } else {
    setSelfHostname(newHostname);
    dashboardHostname = newHostname;
  }

  websetup->join(parseJoinCode(joinCode), dashboardHostname);
}

bool HusarnetManager::isJoined()
{
  auto websetupId = deviceIdFromIpAddress(getWebsetupAddress());

  return (!configStorage->isInternalSettingEmpty(
             InternalSetting::websetupSecret)) &&
         (getLatency(websetupId) >= 0);
}

void HusarnetManager::changeServer(std::string domain)
{
  configStorage->setUserSetting(UserSetting::dashboardFqdn, domain);
  LOG("Dashboard URL has been changed to %s. Daemon will now quit so you can "
      "restart it with new setting.",
      domain.c_str());
  exit(0);
}

void HusarnetManager::hostTableAdd(std::string hostname, IpAddress address)
{
  configStorage->hostTableAdd(hostname, address);
}

void HusarnetManager::hostTableRm(std::string hostname)
{
  configStorage->hostTableRm(hostname);
}

void HusarnetManager::whitelistAdd(IpAddress address)
{
  configStorage->whitelistAdd(address);
}

void HusarnetManager::whitelistRm(IpAddress address)
{
  configStorage->whitelistRm(address);
}

std::list<IpAddress> HusarnetManager::getWhitelist()
{
  return configStorage->getWhitelist();
}

bool HusarnetManager::isWhitelistEnabled()
{
  return configStorage->getUserSettingBool(UserSetting::enableWhitelist);
}

void HusarnetManager::whitelistEnable()
{
  configStorage->setUserSetting(UserSetting::enableWhitelist, true);
}

void HusarnetManager::whitelistDisable()
{
  configStorage->setUserSetting(UserSetting::enableWhitelist, false);
}

bool HusarnetManager::isPeerAddressAllowed(IpAddress address)
{
  if(!isWhitelistEnabled()) {
    return true;
  }

  return configStorage->isOnWhitelist(address);
}

bool HusarnetManager::isRealAddressAllowed(InetAddress addr)
{
  return true;
}

int HusarnetManager::getApiPort()
{
  return configStorage->getUserSettingInt(UserSetting::daemonApiPort);
}

std::string HusarnetManager::getApiSecret()
{
  return Privileged::readApiSecret();
}

std::string HusarnetManager::rotateApiSecret()
{
  Privileged::rotateApiSecret();
  return getApiSecret();
}

std::string HusarnetManager::getDashboardFqdn()
{
  return license->getDashboardFqdn();
}
IpAddress HusarnetManager::getWebsetupAddress()
{
  return license->getWebsetupAddress();
}
std::vector<IpAddress> HusarnetManager::getBaseServerAddresses()
{
  return license->getBaseServerAddresses();
}

NgSocket* HusarnetManager::getNGSocket()
{
  return ngsocket;
}

SecurityLayer* HusarnetManager::getSecurityLayer()
{
  return securityLayer;
}

std::string HusarnetManager::getInterfaceName()
{
  return configStorage->getUserSetting(UserSetting::interfaceName);
}

std::vector<DeviceId> HusarnetManager::getMulticastDestinations(DeviceId id)
{
  if(!id == deviceIdFromIpAddress(IpAddress::parse(multicastDestination))) {
    return {};
  }

  std::vector<DeviceId> r;
  for(auto& ipAddress : configStorage->getWhitelist()) {
    r.push_back(deviceIdFromIpAddress(ipAddress));
  }
  return r;
}

int HusarnetManager::getLatency(DeviceId destination)
{
  return securityLayer->getLatency(destination);
}

void HusarnetManager::cleanup()
{
  configStorage->groupChanges([&]() {
    configStorage->whitelistClear();
    configStorage->whitelistAdd(getWebsetupAddress());
    configStorage->hostTableClear();
  });
}

HusarnetManager::HusarnetManager()
{
  Port::init();
  Privileged::init();
}

void HusarnetManager::getLicenseStage()
{
  license =
      new License(configStorage->getUserSetting(UserSetting::dashboardFqdn));
}

void HusarnetManager::getIdentityStage()
{
  // TODO long term - reenable the smartcard support but with proper
  // multiplatform support
  identity = Privileged::readIdentity();
}

void HusarnetManager::startNetworkingStack()
{
  peerContainer = new PeerContainer(this);

  auto tunTap = Port::startTunTap(this);

  Privileged::dropCapabilities();

  auto multicast = new MulticastLayer(this);
  auto compression = new CompressionLayer(this);
  securityLayer = new SecurityLayer(this);
  ngsocket = new NgSocket(this);

  stackHigherOnLower(tunTap, multicast);
  stackHigherOnLower(multicast, compression);
  stackHigherOnLower(compression, securityLayer);
  stackHigherOnLower(securityLayer, ngsocket);
}

void HusarnetManager::startWebsetup()
{
  websetup = new WebsetupConnection(this);
  websetup->start();
}

void HusarnetManager::startHTTPServer()
{
#ifdef HTTP_CONTROL_API
  threadpool.push_back(new std::thread([this]() {
    auto server = new ApiServer(this);
    server->runThread();
  }));
#endif
}

void HusarnetManager::stage1()
{
  if(stage1Started) {
    return;
  }

  GIL::init();
  Privileged::start();

  logManager = new LogManager(100);
  globalLogManager = logManager;

  configStorage = new ConfigStorage(
      this, Privileged::readConfig, Privileged::writeConfig, userDefaults,
      Port::getEnvironmentOverrides(), internalDefaults);

  selfFlags = new PeerFlags();

  stage1Started = true;
}

void HusarnetManager::stage2()
{
  if(stage2Started) {
    return;
  }

  getLicenseStage();
  getIdentityStage();

  stage2Started = true;
}

void HusarnetManager::stage3()
{
  if(stage3Started) {
    return;
  }

  startNetworkingStack();
  startWebsetup();
  startHTTPServer();

  if(configStorage->isUserSettingOverriden(UserSetting::dashboardFqdn) &&
     configStorage->getPersistentUserSetting(UserSetting::dashboardFqdn) !=
         configStorage->getUserSetting(UserSetting::dashboardFqdn)) {
    changeServer(configStorage->getUserSetting(UserSetting::dashboardFqdn));
  }

  if(configStorage->isUserSettingOverriden(UserSetting::joinCode)) {
    joinNetwork(configStorage->getUserSetting(UserSetting::joinCode));
  }

  stage3Started = true;
}

void HusarnetManager::runHusarnet()
{
  stage1();
  stage2();
  stage3();

  Privileged::notifyReady();

  while(true) {
    ngsocket->periodic();
    OsSocket::runOnce(1000);  // process socket events for at most so many ms
  }
}
