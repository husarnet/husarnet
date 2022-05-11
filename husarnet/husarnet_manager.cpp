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

// TODO
void HusarnetManager::updateHosts()
{
  std::vector<std::pair<IpAddress, std::string>> hosts;
  std::unordered_set<std::string> mappedHosts;

  // for (std::string netId : configTable->listNetworks()) {
  //   for (auto row : configTable->listValuesForNetwork(netId, "host")) {
  //     hosts.push_back({IpAddress::parse(row.value), row.key});
  //     mappedHosts.insert(row.key);
  //   }
  // }

  // {
  //   // TODO: this should be configurable.
  //   // Ensure "master" and hostname of device are always mapped.
  //   if (mappedHosts.find("master") == mappedHosts.end())
  //     hosts.push_back({IpAddress::parse("::1"), "master"});

  //   std::string my_hostname = configGet("manual", "hostname", "");

  //   if (my_hostname.size() > 0)
  //     if (mappedHosts.find(my_hostname) == mappedHosts.end())
  //       hosts.push_back({IpAddress::parse("::1"), my_hostname});
  // }

  // std::sort(hosts.begin(), hosts.end());
  // hostsFileUpdateFunc(hosts);  TODO  link this properly (privileged
  // something)
  // TODO implement this
}

IpAddress HusarnetManager::resolveHostname(std::string hostname)
{
  // auto values = configTable->getValue("host", hostname);
  // if (values.size() > 0)
  //   return IpAddress::parse(values[0]);
  // else
  //   return IpAddress{};
  return IpAddress{};  // TODO fix this
}

// TODO implement this maybe
IpAddress HusarnetManager::getCurrentBaseAddress()
{
  return IpAddress();
}

// TODO implement this maybe
std::string HusarnetManager::getCurrentBaseProtocol()
{
  return "";
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
  whitelistAdd(license->getWebsetupAddress());

  auto newWebsetupSecret =
      setWebsetupSecret(encodeHex(generateRandomString(12)));

  websetup->sendJoinRequest(
      parseJoinCode(joinCode), newWebsetupSecret, newHostname);
}

// TODO implement this maybe
bool HusarnetManager::isJoined()
{
  return false;
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
  websetup->init();
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
