// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/privileged_interface.h"

#include <initializer_list>
#include <map>
#include <utility>

#include <assert.h>
#include <fcntl.h>
#include <linux/capability.h>
#include <linux/securebits.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/unix/privileged_process.h"

#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/util.h"

#include "nlohmann/json.hpp"
#include "stdio.h"

using namespace nlohmann;  // json

// This file needs some explanation (specific to the Unix/Linux implementation)
// First of all the code in the main thread is expected to drop as much
// capabilities as possible, including chrooting itself to /var/lib/husarnet.
// This has a couple of implications, like - we no longer have access to /proc,
// /dev, /etc and other important directories. On order to continue to be able
// to do some things in those spaces, we fork before we drop capabilities and
// are letting the child to retain those "superpowers". That process is,
// unsurprisingly, called `privileged_process` and all of it's code is in a
// separate file. Communication with it is limited to a single, unnamed unix
// socket and encapsulated using JSON. JSON is definitely like using a
// sledgehammer to crack a nut, but 1) we're already using it in another part of
// the codebase 2) it's easy 3) this interface is super low QPS one so let it
// slide.
// As per the implementation details - due to the fact that we chroot after we
// drop capabilities are paths in the main process are skewed, thus `configDir`
// variable is introduced and it's content will change during the lifetime of
// the process. Also - because of that we are still able to manipulate files in
// the /var/lib/husarnet directory from the main process, but 1) not any other
// directories 2) as I written before - they're under `/` path and not
// `/var/lib/husarnet`.

static FILE* if_inet6;

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

static std::string configDir = "/var/lib/husarnet/";

static int privilegedProcessFd = 0;

#ifdef UT
static json callPrivilegedProcess(PrivilegedMethod endpoint, json data)
{
  // disable privileged process while unit testing core
  (void)endpoint;
  (void)data;
  return json::parse("{}");
}
#else
static json callPrivilegedProcess(PrivilegedMethod endpoint, json data)
{
  assert(privilegedProcessFd != 0);

  json frame = {
      {"endpoint", endpoint._to_string()},
      {"data", data},
  };

  auto frameStr = frame.dump(0);
  if(send(privilegedProcessFd, frameStr.data(), frameStr.size() + 1, 0) < 0) {
    perror("send");
    exit(1);
  }

  char recvBuffer[40000];

  if(recv(privilegedProcessFd, recvBuffer, sizeof(recvBuffer), 0) < 0) {
    perror("recv");
    exit(1);
  }

  auto response = json::parse(recvBuffer);
  return response;
}
#endif

struct cap_header_struct {
  __u32 version;
  int pid;  // cppcheck-suppress unusedStructMember
};

struct cap_data_struct {
  __u32 effective;
  __u32 permitted;
  __u32 inheritable;
};

static int set_cap(int flags)
{
  cap_header_struct capheader = {_LINUX_CAPABILITY_VERSION_1, 0};
  cap_data_struct capdata;
  capdata.inheritable = capdata.permitted = capdata.effective = flags;
  return (int)syscall(SYS_capset, &capheader, &capdata);
}

namespace Privileged {
  void init()
  {
    if_inet6 = fopen("/proc/self/net/if_inet6", "r");
    mkdir(configDir.c_str(), 0700);
    chmod(configDir.c_str(), 0700);
  }

  void start()
  {
    int fds[2];  // end for parent, end for privileged subprocess
    if(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds) < 0) {
      perror("socketpair");
      exit(1);
    }

    if(fork() == 0) {
      // Privileged
      close(fds[0]);

      auto privilegedProcess = new PrivilegedProcess();
      privilegedProcess->init(fds[1]);
      privilegedProcess->run();
      _exit(1);
    } else {
      // Parent
      close(fds[1]);
      privilegedProcessFd = fds[0];

      // Technically this call should be done here but it makes more sense to do
      // so after interfaces are configured, so it's exposed to upper layers.

      // dropCapabilities();
    }
  }

  void dropCapabilities()
  {
    if(chroot(configDir.c_str()) < 0) {
      perror("chroot");
      exit(1);
    }

    if(chdir("/") < 0) {
      perror("chdir");
      exit(1);
    }

    configDir = "/";  // this is true only after we do the chroot

    if(prctl(PR_SET_SECUREBITS, SECBIT_KEEP_CAPS | SECBIT_NOROOT) < 0) {
      perror("prctl");
      exit(1);
    }

    if(set_cap(0) < 0) {
      perror("set_cap");
      exit(1);
    }
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

    if(!Port::isFile(identityPath)) {
      auto identity = Identity::create();
      Privileged::writeIdentity(identity);
      return identity;
    }

    auto identity = Identity::deserialize(Port::readFile(identityPath));

    if(!identity.isValid()) {
      identity = Identity::create();
      Privileged::writeIdentity(identity);
    }

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
    return callPrivilegedProcess(PrivilegedMethod::getSelfHostname, {});
  }

  // TODO long term - prevent websetup from renaming this host for no reason
  bool setSelfHostname(const std::string& newHostname)
  {
    return callPrivilegedProcess(
        PrivilegedMethod::setSelfHostname, newHostname);
  }

  void updateHostsFile(const std::map<std::string, IpAddress>& data)
  {
    std::map<std::string, std::string> dataStringified;

    for(auto [hostname, address] : data) {
      dataStringified.insert({hostname, address.toString()});
    }

    callPrivilegedProcess(PrivilegedMethod::updateHostsFile, dataStringified);
  }

  void notifyReady()
  {
    callPrivilegedProcess(PrivilegedMethod::notifyReady, {});
  }
}  // namespace Privileged
