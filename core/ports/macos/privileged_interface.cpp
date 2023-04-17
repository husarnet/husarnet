// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/privileged_interface.h"

#include <initializer_list>
#include <map>
#include <utility>

#include <assert.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
// #include <sys/syscall.h>
#include <unistd.h>

#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/shared_unix_windows/hosts_file_manipulation.h"
#include "husarnet/ports/unix/privileged_process.h"

#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/util.h"

#include "nlohmann/json.hpp"
#include "stdio.h"

using namespace nlohmann;  // json

// MacOS doesn't have capabilities...

static bool isInterfaceBlacklisted(std::string name)
{
  if(name.size() > 2 && name.substr(0, 2) == "zt")
    return true;  // sending trafiic over ZT may cause routing loops

  if(name == "hnetl2")
    return true;

  return false;
}

static void getLocalIpv4Addresses(std::vector<IpAddress>& ret)
{
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  struct ifconf configuration {};

  if(fd < 0)
    return;

  if(ioctl(fd, SIOCGIFCONF, &configuration) < 0)
    goto error;
  configuration.ifc_buf = (char*)malloc(configuration.ifc_len);
  if(ioctl(fd, SIOCGIFCONF, &configuration) < 0)
    goto error;

  for(int i = 0; i < (int)(configuration.ifc_len / sizeof(ifreq)); i++) {
    struct ifreq& request = configuration.ifc_req[i];
    std::string ifname = request.ifr_name;
    if(isInterfaceBlacklisted(ifname))
      continue;
    struct sockaddr* addr = &request.ifr_ifru.ifru_addr;
    if(addr->sa_family != AF_INET)
      continue;

    uint32_t addr32 = ((sockaddr_in*)addr)->sin_addr.s_addr;
    IpAddress address = IpAddress::fromBinary4(addr32);

    if(addr32 == 0x7f000001u)
      continue;

    ret.push_back(address);
  }
  free(configuration.ifc_buf);
error:
  close(fd);
  return;
}

static void getLocalIpv6Addresses(std::vector<IpAddress>& ret)
{
  // on MacOS there is no /proc, so we can't use /proc/self/net/if_inet6
  // like in linux port; need to figure out different way
}

static std::string configDir = "/var/lib/husarnet/";

namespace Privileged {
  void init()
  {
    mkdir(configDir.c_str(), 0700);
    chmod(configDir.c_str(), 0700);
  }

  void start()
  {
    // there is no separate privileged thread on MacOS (rn)
  }

  void dropCapabilities()
  {
    // no cap dropping on MacOS
  }

  std::string getConfigPath()
  {
    return configDir + "config.json";
  }

  std::string getIdentityPath()
  {
    return configDir + "id";
  }

  std::string getApiSecretPath()
  {
    return configDir + "daemon_api_token";
  }

  std::string getLegacyConfigPath()
  {
    return configDir + "config.db";
  }

  std::string getLicenseJsonPath()
  {
    return configDir + "license.json";
  }

  std::string getNotificationFilePath()
  {
    return configDir + "notifications.json";
  }


  std::string readLicenseJson()
  {
    auto licenseJsonPath = getLicenseJsonPath();

    if(!Port::isFile(licenseJsonPath)) {
      return "{}";
    }

    return Port::readFile(licenseJsonPath);
  }

  void writeLicenseJson(std::string data)
  {
    Port::writeFile(getLicenseJsonPath(), data);
  }

  std::string readNotificationFile()
  {
    auto notificationFilePath = getNotificationFilePath();

    if(!Port::isFile(notificationFilePath)) {
      return "{}";
    }

    return Port::readFile(notificationFilePath);
  }

  void writeNotificationFile(std::string data)
  {
    Port::writeFile(getNotificationFilePath(), data);
  }

  std::string readConfig()
  {
    auto configPath = getConfigPath();

    if(!Port::isFile(configPath)) {
      return "";
    }

    return Port::readFile(configPath);
  }

  void writeConfig(std::string data)
  {
    Port::writeFile(getConfigPath(), data);
  }

  Identity readIdentity()
  {
    auto identityPath = getIdentityPath();
    auto identity = Identity::deserialize(Port::readFile(identityPath));
    return identity;
  }

  bool checkValidIdentityExists()
  {
    auto identityPath = getIdentityPath();

    if(!Port::isFile(identityPath)) {
      return false;
    }

    auto identity = Identity::deserialize(Port::readFile(identityPath));

    if(!identity.isValid()) {
      return false;
    }

    return true;
  }

  Identity createIdentity()
  {
    auto identity = Identity::create();
    Privileged::writeIdentity(identity);
    return identity;
  }

  void writeIdentity(Identity identity)
  {
    Port::writeFile(getIdentityPath(), identity.serialize());
  }

  std::string readApiSecret()
  {
    auto apiSecretPath = getApiSecretPath();

    if(!Port::isFile(apiSecretPath)) {
      rotateApiSecret();
    }

    return Port::readFile(apiSecretPath);
  }

  void rotateApiSecret()
  {
    Port::writeFile(getApiSecretPath(), generateRandomString(32));
  }

  std::vector<IpAddress> getLocalAddresses()
  {
    std::vector<IpAddress> ret;

    getLocalIpv4Addresses(ret);
    getLocalIpv6Addresses(ret);

    return ret;
  }

  std::string getSelfHostname()
  {
    // TODO: implement
    return "macos-device";
  }

  // TODO long term - prevent websetup from renaming this host for no reason
  bool setSelfHostname(const std::string& newHostname)
  {
    // TODO: implement
    return true;
  }

  void updateHostsFile(const std::map<std::string, IpAddress>& data)
  {
    updateHostsFileInternal(data);
  }

  void notifyReady()
  {
    // TODO: implement
  }

  void runScripts(const std::string& path)
  {
    // TODO: implement MAC hooks at some point?
  }

  bool checkScriptsExist(const std::string& path)
  {
    return false;
  }

}  // namespace Privileged
