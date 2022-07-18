#include <vector>
#include <chrono>
#include <vector>

#include "husarnet/ports/port.h"
#include "husarnet/ports/threads_port.h"
#include "husarnet/ports/sockets.h"
#include "husarnet/util.h"

#include "shlwapi.h"

static std::string configDir;

std::string getConfigDirPath()
{
  return std::string(getenv("PROGRAMDATA")) + "/husarnet/";
}

static std::string getConfigPath()
{
  return configDir + "/config.json";
}

static std::string getIdentityPath()
{
  return configDir + "/id";
}

static std::string getApiSecretPath()
{
  return configDir + "/api_secret";
}

namespace Privileged {

  void init()
  {
    configDir = getConfigDirPath();
    if(!Port::isFile(configDir)) {
      CreateDirectory(configDir.c_str(), NULL);
      // fixPermissions(configDir);
      // FileStorage::generateAndWriteId(configDir);
      // ympek TODO
      // here we will need to cover a bunch of cases like
      // old config present->run migrator, , new config present, etc.
    }
  }

  void start()
  {
    // there is no separate privileged thread on Windows (rn)
  }

  void dropCapabilities()
  {
    // there is no separate privileged thread on Windows (rn)
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
    // this takes 18 ms on a test system
    std::vector<IpAddress> result;

    int ret;
    PIP_ADAPTER_ADDRESSES buffer;
    unsigned long buffer_size = 20000;

    while(true) {
      buffer = (PIP_ADAPTER_ADDRESSES) new char[buffer_size];

      ret = GetAdaptersAddresses(
          AF_UNSPEC,
          GAA_FLAG_INCLUDE_ALL_INTERFACES | GAA_FLAG_SKIP_FRIENDLY_NAME |
          GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST |
          GAA_FLAG_SKIP_ANYCAST,
          0, buffer, &buffer_size);
      if(ret != ERROR_BUFFER_OVERFLOW)
        break;
      delete[] buffer;
      buffer = nullptr;
      buffer_size *= 2;
    }

    PIP_ADAPTER_ADDRESSES current_adapter = buffer;
    while(current_adapter) {
      PIP_ADAPTER_UNICAST_ADDRESS current = current_adapter->FirstUnicastAddress;
      while(current) {
        auto ss = reinterpret_cast<sockaddr_storage*>(current->Address.lpSockaddr);
        InetAddress addr = OsSocket::ipFromSockaddr(*ss);
        // LOG("detected IP: %s", addr.str().c_str());
        result.push_back(addr.ip);
        current = current->Next;
      }
      current_adapter = current_adapter->Next;
    }

    delete[] buffer;
    buffer = nullptr;
    return result;
  }

  std::string getSelfHostname()
  {
    // TODO ympek implement
    return "";
    // return callPrivilegedProcess(PrivilegedMethod::getSelfHostname, {});
  }

  // TODO long term - prevent websetup from renaming this host for no reason
  bool setSelfHostname(std::string newHostname)
  {
    // just do it here

    // return callPrivilegedProcess(
    //     PrivilegedMethod::setSelfHostname, newHostname);
    return true;
  }

  void updateHostsFile(std::map<std::string, IpAddress> data)
  {
    std::map<std::string, std::string> dataStringified;

    for(auto& [hostname, address] : data) {
      dataStringified.insert({hostname, address.toString()});
    }

    // just do it here
    // callPrivilegedProcess(PrivilegedMethod::updateHostsFile, dataStringified);
  }

  void notifyReady()
  {
    // callPrivilegedProcess(PrivilegedMethod::notifyReady, {});
  }
}
