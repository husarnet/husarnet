// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "privileged_interface.h"

#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include "husarnet/util.h"

namespace fs = std::filesystem;

// TODO this whole file

static FILE* if_inet6;

// TODO make it sane or something - this must be called early
static void beforeDropCap()
{
  if_inet6 = fopen("/proc/self/net/if_inet6", "r");
}

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
  bool fileOpenedManually = if_inet6 == nullptr;
  FILE* f =
      fileOpenedManually ? fopen("/proc/self/net/if_inet6", "r") : if_inet6;
  if(f == nullptr) {
    LOG("failed to open if_inet6 file");
    return;
  }
  fseek(f, 0, SEEK_SET);
  while(true) {
    char buf[100];
    if(fgets(buf, sizeof(buf), f) == nullptr)
      break;
    std::string line = buf;
    if(line.size() < 32)
      continue;
    auto ip = IpAddress::fromBinary(decodeHex(line.substr(0, 32)));
    if(ip.isLinkLocal())
      continue;
    if(ip == IpAddress::fromBinary("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1"))
      continue;

    std::string ifname = line.substr(44);
    while(ifname.size() && ifname[0] == ' ')
      ifname = ifname.substr(1);
    if(isInterfaceBlacklisted(ifname))
      continue;

    ret.push_back(ip);
  }
  if(fileOpenedManually)
    fclose(f);
}

static bool writeFile(std::string path, std::string data)
{
  FILE* f = fopen((path + ".tmp").c_str(), "wb");
  int ret = fwrite(data.data(), data.size(), 1, f);
  if(ret != 1) {
    LOG("could not write to %s (failed to write to temporary file)",
        path.c_str());
    return false;
  }
  fsync(fileno(f));
  fclose(f);

  if(rename((path + ".tmp").c_str(), path.c_str()) < 0) {
    LOG("could not write to %s (rename failed)", path.c_str());
    return false;
  }
  return true;
}

static std::string readFile(std::string path)
{
  std::ifstream f(path);
  if(!f.good()) {
    LOG("failed to open %s", path.c_str());
    exit(1);
  }

  std::stringstream buffer;
  buffer << f.rdbuf();

  return buffer.str();
}

static bool existsFile(std::string path)
{
  return fs::exists(path);
}

static bool validateHostname(std::string hostname)
{
  if(hostname.size() == 0)
    return false;
  for(char c : hostname) {
    bool ok = ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
              ('0' <= c && c <= '9') || (c == '_' || c == '-' || c == '.');
    if(!ok) {
      return false;
    }
  }
  return true;
}

const static std::string configPath = "/var/lib/husarnet/config.json";
const static std::string identityPath = "/var/lib/husarnet/id";
const static std::string apiSecretPath = "/var/lib/husarnet/api_secret";
const static std::string hostnamePath = "/etc/hostname";
const static std::string hostsPath = "/etc/hosts";
// look for getHostsFilePath in the old code for windows paths

namespace Privileged {
  void init()
  {
  }

  void start()
  {
    beforeDropCap();
  }

  std::string readConfig()
  {
    if(!existsFile(configPath)) {
      return "";
    }

    return readFile(configPath);
  }

  void writeConfig(std::string data)
  {
    writeFile(configPath, data);
  }

  Identity readIdentity()
  {
    if(!existsFile(identityPath)) {
      auto identity = Identity::create();
      Privileged::writeIdentity(identity);
      return identity;
    }

    auto identity = Identity::deserialize(readFile(identityPath));

    if(!identity.isValid()) {
      identity = Identity::create();
      Privileged::writeIdentity(identity);
    }

    return identity;
  }

  void writeIdentity(Identity identity)
  {
    writeFile(identityPath, identity.serialize());
  }

  std::string readApiSecret()
  {
    if(!existsFile(apiSecretPath)) {
      rotateApiSecret();
    }

    return readFile(apiSecretPath);
  }

  void rotateApiSecret()
  {
    writeFile(apiSecretPath, generateRandomString(32));
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
    return rtrim(readFile(hostnamePath));
  }

  bool setSelfHostname(std::string newHostname)
  {
    if(!validateHostname(newHostname)) {
      return false;
    }

    writeFile(hostnamePath, newHostname);
    return true;
  }

  void updateHostsFile(std::map<std::string, IpAddress> data)
  {
  }
}  // namespace Privileged
