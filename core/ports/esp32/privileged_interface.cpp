// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "lwip/netif.h"

static std::string configDir;

namespace Privileged {

  void init()
  {
  }

  void start()
  {
  }

  void createConfigDirectories()
  {
  }

  void dropCapabilities()
  {
    // Technically this port will always be running as privileged
  }

  // Following functions return keys for the NVS storage
  std::string getConfigPath()
  {
    return "husarnet_config";
  }

  std::string getIdentityPath()
  {
    return "husarnet_id";
  }

  std::string getApiSecretPath()
  {
    return "husarnet_api_secret";
  }

  // std::string getLegacyConfigPath()
  // {
  //   return "husarnet_config_legacy"; // not used
  // }

  std::string getLicenseJsonPath()
  {
    return "husarnet_license";
  }

  // TODO: Notifications are not implemented yet,
  // decide if we want to keep them in the ESP32 port
  std::string getNotificationFilePath()
  {
    return "husarnet_notifications";
  }

  std::string readNotificationFile()
  {
    LOG_WARNING("readNotificationFile not implemented");
    return "";
  }

  void writeNotificationFile(std::string data)
  {
    LOG_WARNING("writeNotificationFile not implemented");
  }

  std::string readLicenseJson()
  {
    return Port::readFile(Privileged::getLicenseJsonPath());
  }

  void writeLicenseJson(std::string data)
  {
    Port::writeFile(Privileged::getLicenseJsonPath(), data);
  }

  std::string readConfig()
  {
    return Port::readFile(Privileged::getConfigPath());
  }

  void writeConfig(std::string data)
  {
    Port::writeFile(Privileged::getConfigPath(), data);
  }

  Identity readIdentity()
  {
    return Identity::deserialize(Port::readFile(Privileged::getIdentityPath()));
  }

  Identity createIdentity() {
    auto identity = Identity::create();
    Privileged::writeIdentity(identity);
    return identity;
  }

  bool checkValidIdentityExists() {
    auto identity = readIdentity();

    if(!identity.isValid()) {
      return false;
    }

    return true;
  }

  void writeIdentity(Identity identity)
  {
    Port::writeFile(Privileged::getIdentityPath(), identity.serialize());
  }

  IpAddress resolveToIp(const std::string& hostname)
  {
    return Port::resolveToIp(hostname);
  }

  std::string readApiSecret()
  {
    return Port::readFile(Privileged::getApiSecretPath());
  }

  void rotateApiSecret()
  {
    Port::writeFile(Privileged::getApiSecretPath(), generateRandomString(32));
  }

  // TODO: Notifications are not implemented yet,
  // decide if we want to keep them in the ESP32 port
  std::vector<std::pair<std::time_t, std::string>> readNotifications()
  {
    LOG_WARNING("readNotifications not implemented");
    return {};
  }

  void writeNotifications(std::vector<std::pair<std::time_t, std::string>> list)
  {
    LOG_WARNING("writeNotifications not implemented");
  }

  std::vector<IpAddress> getLocalAddresses()
  {
    std::vector<IpAddress> ret;
    netif* netif;

    for (u8_t idx = 1; (netif = netif_get_by_index(idx)) != NULL; idx++) {
      char buf[IP6ADDR_STRLEN_MAX];
      const ip6_addr_t* ipv6;
      // Iterate over all IPv6 addresses and validate them
      for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        ipv6 = netif_ip6_addr(netif, i);
        if (ipv6 == NULL) {
          continue;
        }

        if (ip6_addr_isany(ipv6) || ip6_addr_isloopback(ipv6)) {
          continue;
        }

        // Append to valid addresses
        ip6addr_ntoa_r(ipv6, buf, IP6ADDR_STRLEN_MAX);
        LOG_DEBUG("netif: %s, ip6: %s", netif->name, buf);
        ret.push_back(IpAddress::fromBinary(buf));
      }

      // LwIP can have only one IPv4 address per interface
      // Validate address
      const ip4_addr_t* ipv4 = netif_ip4_addr(netif);
      if (ipv4 == NULL) {
        continue;
      }

      if (ip4_addr_isany(ipv4) || ip4_addr_isloopback(ipv4)) {
        continue;
      }

      // Append to valid addresses
      ip4addr_ntoa_r(ipv4, buf, IP4ADDR_STRLEN_MAX);
      LOG_DEBUG("netif: %s, ip4: %s", netif->name, buf);
      ret.push_back(IpAddress::fromBinary4(buf));
    }

    return ret;
  }

  std::string getSelfHostname()
  {
    return "husarnet-esp32";  // @TODO
  }

  bool setSelfHostname(const std::string& newHostname)
  {
    return false;  // @TODO
  }

  void updateHostsFile(const std::map<std::string, IpAddress>& data)
  {
    // @TODO
  }

  void notifyReady()
  {
    // @TODO
  }

  void runScripts(const std::string& path) {
    LOG_ERROR("runScripts not supported on ESP32");
    // TODO
  }

  bool checkScriptsExist(const std::string& path) {
    return false;
  }
}  // namespace Privileged
