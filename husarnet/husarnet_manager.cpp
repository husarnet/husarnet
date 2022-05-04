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

// TODO remove this method entirely
std::string HusarnetManager::configGet(
    std::string networkId,
    std::string key,
    std::string defaultValue)
{
  // auto v = configTable->getValueForNetwork(networkId, "config", key);
  // if (v.size() == 0) {
  //   return defaultValue;
  // }
  // if (v.size() > 1) {
  //   LOG("warning: multiple values for config key %s", key.c_str());
  // }

  // return v[0];
  return "";  // TODO
}

// TODO remove this method entirely
std::string HusarnetManager::configSet(
    std::string networkId,
    std::string key,
    std::string value)
{
  // configTable->runInTransaction([&]() {
  //   configTable->removeValues(networkId, "config", key);
  //   configTable->insert(ConfigRow{networkId, "config", key, value});
  // });

  // return value;
  return "";  // TODO
}

LogManager& HusarnetManager::getLogManager()
{
  return *(this->logManager);
}

// TODO implement this maybe
std::string HusarnetManager::getVersion()
{
  return "";
}

// TODO implement this maybe
std::string HusarnetManager::getUserAgent()
{
  return "";
}

// TODO implement this maybe
IpAddress HusarnetManager::getSelfAddress()
{
  return IpAddress();
}

std::string HusarnetManager::getSelfHostname()
{
  return configGet("manual", "hostname", "");
}

void HusarnetManager::setSelfHostname(std::string newHostname)
{
  LOG("changing hostname to '%s'", newHostname.c_str());
  configSet("manual", "hostname", newHostname);
  // ServiceHelper::modifyHostname(newHostname); // TODO link this properly
}

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
  return configGet("manual", "websetup-secret", "");
}

std::string HusarnetManager::setWebsetupSecret(std::string newSecret)
{
  configSet("manual", "websetup-secret", newSecret);
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

// TODO implement this maybe
void HusarnetManager::hostTableAdd(std::string hostname, IpAddress address)
{
}

// TODO implement this maybe
void HusarnetManager::hostTableRm(std::string hostname)
{
}

// TODO implement this maybe
void HusarnetManager::whitelistAdd(IpAddress address)
{
}

// TODO implement this maybe
void HusarnetManager::whitelistRm(IpAddress address)
{
}

// TODO implement this maybe
std::list<IpAddress> HusarnetManager::getWhitelist()
{
  return {};
}

// TODO implement this
bool HusarnetManager::isWhitelistEnabled()
{
  return false;
}

// TODO implement this maybe
void HusarnetManager::whitelistEnable()
{
}

// TODO implement this maybe
void HusarnetManager::whitelistDisable()
{
}

bool HusarnetManager::isHostAllowed(IpAddress id)
{
  if(!isWhitelistEnabled()) {
    return true;
  }

  auto whitelist = getWhitelist();
  return std::find(begin(whitelist), end(whitelist), id) != std::end(whitelist);
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

void HusarnetManager::getLicense()
{
  this->license =
      new License(configStorage->getUserSetting(UserSetting::dashboardUrl));
}

void HusarnetManager::getIdentity()
{
  // TODO - reenable the smartcard support but with proper multiplatform support
  identity = Privileged::readIdentity();

  if(!identity.isValid()) {
    identity = Identity::create();
    Privileged::writeIdentity(identity);
  }
}

void HusarnetManager::startNGSocket()
{
  ngsocket = NgSocketSecure::create(&identity, this);
  ngsocket->options->isPeerAllowed = [&](DeviceId id) {
    return isHostAllowed(deviceIdToIpAddress(id));
  };

  ngsocket->options->userAgent = "Linux daemon";
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
  threadpool.push_back(
      new std::thread([this]() { APIServer::httpThread(*this); }));
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

  getLicense();
  getIdentity();
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
