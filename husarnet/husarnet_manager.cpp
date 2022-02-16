#include "ports/port.h"

#include <sstream>
#include "global_lock.h"
#include "husarnet_config.h"
#include "husarnet_crypto.h"
#include "husarnet_manager.h"
#include "ports/sockets.h"
#include "privileged/privileged_interface.h"
#include "util.h"

#ifdef HTTP_CONTROL_API
#include "api_server/server.h"
#endif

std::string HusarnetManager::configGet(std::string networkId,
                                       std::string key,
                                       std::string defaultValue) {
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

std::string HusarnetManager::configSet(std::string networkId,
                                       std::string key,
                                       std::string value) {
  // configTable->runInTransaction([&]() {
  //   configTable->removeValues(networkId, "config", key);
  //   configTable->insert(ConfigRow{networkId, "config", key, value});
  // });

  // return value;
  return "";  // TODO
}

LogManager& HusarnetManager::getLogManager() {
  return *(this->logManager);
}

// TODO implement this maybe
std::string HusarnetManager::getVersion() {
  return "";
}

// TODO implement this maybe
std::string HusarnetManager::getUserAgent() {
  return "";
}

// TODO implement this maybe
IpAddress HusarnetManager::getSelfAddress() {
  return IpAddress();
}

std::string HusarnetManager::getSelfHostname() {
  return configGet("manual", "hostname", "");
}

void HusarnetManager::setSelfHostname(std::string newHostname) {
  LOG("changing hostname to '%s'", newHostname.c_str());
  configSet("manual", "hostname", newHostname);
  // ServiceHelper::modifyHostname(newHostname); // TODO link this properly
}

void HusarnetManager::updateHosts() {
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

IpAddress HusarnetManager::resolveHostname(std::string hostname) {
  // auto values = configTable->getValue("host", hostname);
  // if (values.size() > 0)
  //   return IpAddress::parse(values[0]);
  // else
  //   return IpAddress{};
  return IpAddress{};  // TODO fix this
}

// TODO implement this maybe
IpAddress HusarnetManager::getCurrentBaseAddress() {
  return IpAddress();
}

// TODO implement this maybe
std::string HusarnetManager::getCurrentBaseProtocol() {
  return "";
}

std::string HusarnetManager::getWebsetupSecret() {
  return configGet("manual", "websetup-secret", "");
}

std::string HusarnetManager::setWebsetupSecret(std::string newSecret) {
  configSet("manual", "websetup-secret", newSecret);
  return newSecret;
}

static std::string parseJoinCode(std::string joinCode) {
  int slash = joinCode.find("/");
  if (slash == -1) {
    return joinCode;
  }

  return joinCode.substr(slash + 1);
}

void HusarnetManager::joinNetwork(std::string joinCode,
                                  std::string newHostname) {
  whitelistAdd(license->getWebsetupAddress());

  auto newWebsetupSecret = setWebsetupSecret(encodeHex(randBytes(12)));

  websetup->sendJoinRequest(parseJoinCode(joinCode), newWebsetupSecret,
                            newHostname);
}

// TODO implement this maybe
bool HusarnetManager::isJoined() {
  return false;
}

// TODO implement this maybe
void HusarnetManager::hostTableAdd(std::string hostname, IpAddress address) {}

// TODO implement this maybe
void HusarnetManager::hostTableRm(std::string hostname) {}

// TODO implement this maybe
void HusarnetManager::whitelistAdd(IpAddress address) {}

// TODO implement this maybe
void HusarnetManager::whitelistRm(IpAddress address) {}

// TODO implement this maybe
std::list<IpAddress> HusarnetManager::getWhitelist() {
  return {};
}

// TODO implement this maybe
void HusarnetManager::whitelistEnable() {}

// TODO implement this maybe
void HusarnetManager::whitelistDisable() {}

std::string HusarnetManager::getDashboardUrl() {
  return license->getDashboardUrl();
}
IpAddress HusarnetManager::getWebsetupAddress() {
  return license->getWebsetupAddress();
}
std::vector<IpAddress> HusarnetManager::getBaseServerAddresses() {
  return license->getBaseServerAddresses();
}

std::vector<DeviceId> HusarnetManager::getMulticastDestinations(DeviceId id) {
  if (std::string(id).find("\xff\x15\xf2\xd3\xa3\x89") == 0) {
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
int HusarnetManager::getLatency(IpAddress destination) {
  return 0;
}

void HusarnetManager::cleanup() {
  // configTable->runInTransaction([&]() {
  //   configTable->removeAll("manual", "whitelist");
  //   configTable->removeAll("manual", "host");
  // });
  // TODO fix this
}

HusarnetManager::HusarnetManager() {
  Privileged::init();

  configStorage = new ConfigStorage(
      Privileged::readSettings, Privileged::writeSettings, userDefaults,
      getEnvironmentOverrides(), internalDefaults);
}

void HusarnetManager::runHusarnet() {
  Privileged::start();

  // You need to get this variable as late as possible, so platforms like ESP32
  // can prepopulate config with orverriden data
  auto dashboardHostname =
      configGet("manual", "dashboard-hostname", "app.husarnet.com");
  this->license = new License(dashboardHostname);
  this->websetup = new WebsetupConnection(this);

  websetup->init();

  GIL::startThread([this]() { websetup->run(); }, "websetup", 6000);

#ifdef HTTP_CONTROL_API
  std::thread httpThread([this]() { APIServer::httpThread(*this); });
#endif

  while (true) {
    sock->periodic();
    OsSocket::runOnce(1000);
  }
}
