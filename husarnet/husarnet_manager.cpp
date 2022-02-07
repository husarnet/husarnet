#include "ports/port.h"

#include <sstream>
#include "filestorage.h"
#include "global_lock.h"
#include "husarnet_config.h"
#include "husarnet_crypto.h"
#include "husarnet_manager.h"
#include "ports/sockets.h"
#include "service_helper.h"
#include "util.h"

#ifdef HTTP_CONTROL_API
#include "api_server/server.h"
#endif

static std::string parseJoinCode(std::string joinCode) {
  int slash = joinCode.find("/");
  if (slash == -1) {
    return joinCode;
  }

  return joinCode.substr(slash + 1);
}

std::string HusarnetManager::configGet(std::string networkId,
                                       std::string key,
                                       std::string defaultValue) {
  auto v = configTable->getValueForNetwork(networkId, "config", key);
  if (v.size() == 0) {
    return defaultValue;
  }
  if (v.size() > 1) {
    LOG("warning: multiple values for config key %s", key.c_str());
  }

  return v[0];
}

std::string HusarnetManager::configSet(std::string networkId,
                                       std::string key,
                                       std::string value) {
  configTable->runInTransaction([&]() {
    configTable->removeValues(networkId, "config", key);
    configTable->insert(ConfigRow{networkId, "config", key, value});
  });

  return value;
}

std::vector<DeviceId> HusarnetManager::getMulticastDestinations(DeviceId id) {
  if (std::string(id).find("\xff\x15\xf2\xd3\xa3\x89") == 0) {
    std::vector<DeviceId> res;

    for (auto row : configTable->listValuesForNetwork("manual", "whitelist")) {
      res.push_back(IpAddress::parse(row.key).toBinary());
    }

    return res;
  } else {
    return {};
  }
}

IpAddress HusarnetManager::resolveHostname(std::string hostname) {
  auto values = configTable->getValue("host", hostname);
  if (values.size() > 0)
    return IpAddress::parse(values[0]);
  else
    return IpAddress{};
}

void HusarnetManager::updateHosts() {
  std::vector<std::pair<IpAddress, std::string>> hosts;
  std::unordered_set<std::string> mappedHosts;

  for (std::string netId : configTable->listNetworks()) {
    for (auto row : configTable->listValuesForNetwork(netId, "host")) {
      hosts.push_back({IpAddress::parse(row.value), row.key});
      mappedHosts.insert(row.key);
    }
  }

  {
    // TODO: this should be configurable.
    // Ensure "master" and hostname of device are always mapped.
    if (mappedHosts.find("master") == mappedHosts.end())
      hosts.push_back({IpAddress::parse("::1"), "master"});

    std::string my_hostname = configGet("manual", "hostname", "");

    if (my_hostname.size() > 0)
      if (mappedHosts.find(my_hostname) == mappedHosts.end())
        hosts.push_back({IpAddress::parse("::1"), my_hostname});
  }

  std::sort(hosts.begin(), hosts.end());
  hostsFileUpdateFunc(hosts);
}

std::string HusarnetManager::getWebsetupSecret() {
  return configGet("manual", "websetup-secret", "");
}

std::string HusarnetManager::setWebsetupSecret(std::string newSecret) {
  configSet("manual", "websetup-secret", newSecret);
  return newSecret;
}

void HusarnetManager::joinNetwork(std::string joinCode,
                                  std::string newHostname) {
  whitelistAdd(license->getWebsetupAddress());

  auto newWebsetupSecret = setWebsetupSecret(encodeHex(randBytes(12)));

  websetup->sendJoinRequest(joinCode, newWebsetupSecret, newHostname);
}

std::string HusarnetManager::getSelfHostname() {
  return configGet("manual", "hostname", "");
}

void HusarnetManager::setSelfHostname(std::string newHostname) {
  LOG("changing hostname to '%s'", newHostname.c_str());
  configSet("manual", "hostname", newHostname);
  ServiceHelper::modifyHostname(newHostname);
}

std::string HusarnetManager::getDashboardUrl() {
  return license->getDashboardUrl();
}
IpAddress HusarnetManager::getWebsetupAddress() {
  return license->getWebsetupAddress();
}
std::list<IpAddress> HusarnetManager::getBaseServerAddresses() {
  return license->getBaseServerAddresses();
}

void HusarnetManager::cleanup() {
  configTable->runInTransaction([&]() {
    configTable->removeAll("manual", "whitelist");
    configTable->removeAll("manual", "host");
  });
}

void HusarnetManager::runHusarnet() {
  auto dashboardHostname =
      configGet("manual", "dashboard-hostname", "app.husarnet.com");
  this->license = new License(dashboardHostname);
  this->websetup = new WebsetupConnection(this);

  websetup->init();

  GIL::startThread([this]() { websetup->run(); }, "websetup", 6000);

#ifdef HTTP_CONTROL_API
  std::thread httpThread([this]() { APIServer::httpThread(); },
                         "http_control_api");
#endif

  while (true) {
    sock->periodic();
    OsSocket::runOnce(1000);
  }
}
