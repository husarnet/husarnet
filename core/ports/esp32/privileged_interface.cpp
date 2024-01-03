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

  // TODO: implement hostname resolution
  IpAddress resolveToIp(const std::string& hostname)
  {
    LOG_WARNING("resolveToIp not implemented");
    return IpAddress{};
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

  // TODO: important: implement local addresses
  std::vector<IpAddress> getLocalAddresses()
  {
    // esp_netif_get_all_ip6

         std::vector<IpAddress>
             ret;
    // for(tcpip_adapter_if_t ifid : {TCPIP_ADAPTER_IF_STA, TCPIP_ADAPTER_IF_AP}) {
    //   tcpip_adapter_ip_info_t info;
    //   if(tcpip_adapter_get_ip_info(ifid, &info) == ESP_OK) {
    //     IpAddress addr = IpAddress::fromBinary4(info.ip.addr);
    //     ret.push_back(addr);
    //   }
    // }
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
