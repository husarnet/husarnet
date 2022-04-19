// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "privileged_interface.h"

#include <net/if.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <thread>
#include "husarnet/util.h"

// TODO this whole file

static FILE* if_inet6;

// @TODO make it sane or something - this must be called early
static void beforeDropCap() {
  if_inet6 = fopen("/proc/self/net/if_inet6", "r");
}

static bool isInterfaceBlacklisted(std::string name) {
  if (name.size() > 2 && name.substr(0, 2) == "zt")
    return true;  // sending trafiic over ZT may cause routing loops

  if (name == "hnetl2")
    return true;

  return false;
}

static void getLocalIpv4Addresses(std::vector<IpAddress>& ret) {
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  struct ifconf configuration {};

  if (fd < 0)
    return;

  if (ioctl(fd, SIOCGIFCONF, &configuration) < 0)
    goto error;
  configuration.ifc_buf = (char*)malloc(configuration.ifc_len);
  if (ioctl(fd, SIOCGIFCONF, &configuration) < 0)
    goto error;

  for (int i = 0; i < (int)(configuration.ifc_len / sizeof(ifreq)); i++) {
    struct ifreq& request = configuration.ifc_req[i];
    std::string ifname = request.ifr_name;
    if (isInterfaceBlacklisted(ifname))
      continue;
    struct sockaddr* addr = &request.ifr_ifru.ifru_addr;
    if (addr->sa_family != AF_INET)
      continue;

    uint32_t addr32 = ((sockaddr_in*)addr)->sin_addr.s_addr;
    IpAddress address = IpAddress::fromBinary4(addr32);

    if (addr32 == 0x7f000001u)
      continue;

    ret.push_back(address);
  }
  free(configuration.ifc_buf);
error:
  close(fd);
  return;
}

static void getLocalIpv6Addresses(std::vector<IpAddress>& ret) {
  bool fileOpenedManually = if_inet6 == nullptr;
  FILE* f =
      fileOpenedManually ? fopen("/proc/self/net/if_inet6", "r") : if_inet6;
  if (f == nullptr) {
    LOG("failed to open if_inet6 file");
    return;
  }
  fseek(f, 0, SEEK_SET);
  while (true) {
    char buf[100];
    if (fgets(buf, sizeof(buf), f) == nullptr)
      break;
    std::string line = buf;
    if (line.size() < 32)
      continue;
    auto ip = IpAddress::fromBinary(decodeHex(line.substr(0, 32)));
    if (ip.isLinkLocal())
      continue;
    if (ip == IpAddress::fromBinary("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1"))
      continue;

    std::string ifname = line.substr(44);
    while (ifname.size() && ifname[0] == ' ')
      ifname = ifname.substr(1);
    if (isInterfaceBlacklisted(ifname))
      continue;

    ret.push_back(ip);
  }
  if (fileOpenedManually)
    fclose(f);
}

namespace Privileged {
void init() {}

void start() {
  beforeDropCap();
}

std::string readConfig() {
  return "";
}

void writeConfig(std::string) {}
// namespace Privileged

Identity readIdentity() {
  return Identity();
}

void writeIdentity(Identity identity) {}

std::vector<IpAddress> getLocalAddresses() {
  std::vector<IpAddress> ret;

  getLocalIpv4Addresses(ret);
  getLocalIpv6Addresses(ret);

  return ret;
}

}  // namespace Privileged
