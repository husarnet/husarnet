// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/port.h"

#include <sstream>
#include "husarnet/device_id.h"
#include "husarnet/gil.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/ngsocket_secure.h"
#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/privileged_interface.h"
#include "husarnet/ports/sockets.h"
#include "husarnet/util.h"

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

std::string HusarnetManager::getVersion()
{
  return HUSARNET_VERSION;
}

std::string HusarnetManager::getUserAgent()
{
  return PORT_NAME;
}

Identity* HusarnetManager::getIdentity()
{
  return &identity;
}

IpAddress HusarnetManager::getSelfAddress()
{
  return identity.getIpAddress();
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

// TODO send plain init-request on startup if has websetup secreat so it can
// download whitelist updates
void HusarnetManager::joinNetwork(std::string joinCode, std::string newHostname)
{
  whitelistAdd(getWebsetupAddress());

  auto newWebsetupSecret =
      setWebsetupSecret(encodeHex(generateRandomString(12)));

  std::string dashboardHostname;
  if(newHostname.empty()) {
    dashboardHostname = Privileged::getSelfHostname();
  } else {
    setSelfHostname(newHostname);
    dashboardHostname = newHostname;
  }

  websetup->sendJoinRequest(
      parseJoinCode(joinCode), newWebsetupSecret, dashboardHostname);
}

// TODO add an endporint for cheching whether it has talked to websetup recently
bool HusarnetManager::isJoined()
{
  // This implementation is very naive as it does only check local
  // configuration for data, but it's the best we can do for now
  return !configStorage->isInternalSettingEmpty(
      InternalSetting::websetupSecret);
}

void HusarnetManager::changeServer(std::string domain)
{
  configStorage->setUserSetting(UserSetting::dashboardUrl, domain);
  exit(1);
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
  return configStorage->getUserSettingBool(UserSetting::whitelistEnable);
}

void HusarnetManager::whitelistEnable()
{
  configStorage->setUserSetting(UserSetting::whitelistEnable, true);
}

void HusarnetManager::whitelistDisable()
{
  configStorage->setUserSetting(UserSetting::whitelistEnable, false);
}

bool HusarnetManager::isHostAllowed(IpAddress id)
{
  if(!isWhitelistEnabled()) {
    return true;
  }

  auto whitelist = getWhitelist();
  // TODO it'd be wise to optimize it (use hashmaps or sth)
  return std::find(begin(whitelist), end(whitelist), id) != std::end(whitelist);
}

int HusarnetManager::getApiPort()
{
  return configStorage->getUserSettingInt(UserSetting::apiPort);
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

std::string HusarnetManager::getDashboardUrl()
{
  return license->getDashboardUrl();
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

std::string HusarnetManager::getInterfaceName()
{
  return configStorage->getUserSetting(UserSetting::interfaceName);
}

std::vector<DeviceId> HusarnetManager::getMulticastDestinations(DeviceId id)
{
  if(std::string(id).find("\xff\x15\xf2\xd3\xa3\x89") == 0) {
    std::vector<DeviceId> res;

    // for (auto row : configTable->listValuesForNetwork("manual", "whitelist"))
    // {
    //   res.push_back(IpAddress::parse(row.key).toBinary());
    // }
    // TODO fix this

    return res;
  } else {
    return {};
  }
}

// TODO implement this maybe
int HusarnetManager::getLatency(IpAddress destination)
{
  return 0;
}

void HusarnetManager::cleanup()
{
  // configTable->runInTransaction([&]() {
  //   configTable->removeAll("manual", "whitelist");
  //   configTable->removeAll("manual", "host");
  // });
  // TODO fix this
}

HusarnetManager::HusarnetManager()
{
  Port::init();
  Privileged::init();
}

void HusarnetManager::getLicenseStage()
{
  this->license =
      new License(configStorage->getUserSetting(UserSetting::dashboardUrl));
}

void HusarnetManager::getIdentityStage()
{
  // TODO - reenable the smartcard support but with proper multiplatform support
  identity = Privileged::readIdentity();
}

void HusarnetManager::startNGSocket()
{
  ngsocket = NgSocketSecure::create(&identity, this);
  ngsocket->options->isPeerAllowed = [&](DeviceId id) {
    return isHostAllowed(deviceIdToIpAddress(id));
  };

  ngsocket->options->userAgent = getUserAgent();

  Port::startTunTap(this);
}

void HusarnetManager::startWebsetup()
{
  this->websetup = new WebsetupConnection(this);
  GIL::startThread([this]() { websetup->run(); }, "websetup", 6000);
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
      Privileged::readConfig, Privileged::writeConfig, userDefaults,
      getEnvironmentOverrides(), internalDefaults);
}

void HusarnetManager::stage2()
{
  if(stage2Started) {
    return;
  }

  getLicenseStage();
  getIdentityStage();
}

void HusarnetManager::stage3()
{
  if(stage3Started) {
    return;
  }

  startNGSocket();
  startWebsetup();
  startHTTPServer();
}

void HusarnetManager::runHusarnet()
{
  stage1();
  stage2();
  stage3();

  while(true) {
    ngsocket->periodic();
    OsSocket::runOnce(1000);
  }
}
