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

#include "esp_netif.h"

static std::string configDir;

namespace Privileged {

  void init()
  {
  }

  void start()
  {
  }

  void dropCapabilities()
  {
    // Technically this port will always be running as privileged
  }

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

  std::string getLicenseJsonPath()
  {
    return "husarnet_license";
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

  void writeIdentity(Identity identity)
  {
    Port::writeFile(Privileged::getIdentityPath(), identity.serialize());
  }

  std::string readApiSecret()
  {
    return Port::readFile(Privileged::getApiSecretPath());
  }

  void rotateApiSecret()
  {
    Port::writeFile(Privileged::getApiSecretPath(), generateRandomString(32));
  }

  std::vector<IpAddress> getLocalAddresses()
  {
    esp_netif_get_all_ip6

    std::vector<IpAddress> ret;
    for(tcpip_adapter_if_t ifid : {TCPIP_ADAPTER_IF_STA, TCPIP_ADAPTER_IF_AP}) {
      tcpip_adapter_ip_info_t info;
      if(tcpip_adapter_get_ip_info(ifid, &info) == ESP_OK) {
        IpAddress addr = IpAddress::fromBinary4(info.ip.addr);
        ret.push_back(addr);
      }
    }
    return ret;
  }

  std::string getSelfHostname()
  {
    return "";  // @TODO
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
}  // namespace Privileged
