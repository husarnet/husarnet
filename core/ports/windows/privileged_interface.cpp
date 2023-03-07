// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>


#include "husarnet/ports/port.h"
#include "husarnet/ports/shared_unix_windows/hosts_file_manipulation.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "shlwapi.h"
#include "sysinfoapi.h"

static std::string configDir;

namespace Privileged {

  void init()
  {
    configDir = std::string(getenv("PROGRAMDATA")) + "\\husarnet";
    if(!Port::isFile(configDir)) {
      CreateDirectory(configDir.c_str(), NULL);
      // fixPermissions(configDir);
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

  std::string getConfigPath()
  {
    return configDir + "\\config.json";
  }

  std::string getIdentityPath()
  {
    return configDir + "\\id";
  }

  std::string getApiSecretPath()
  {
    return configDir + "\\daemon_api_token";
  }

  std::string getLegacyConfigPath()
  {
    return configDir + "\\config.db";
  }

  std::string getLicenseJsonPath()
  {
    return configDir + "\\license.json";
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
      // TODO log error?
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
    // this takes 18 ms on a test system
    std::vector<IpAddress> result;

    PIP_ADAPTER_ADDRESSES buffer;
    unsigned long buffer_size = 20000;

    while(true) {
      int ret;

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
      PIP_ADAPTER_UNICAST_ADDRESS current =
          current_adapter->FirstUnicastAddress;
      while(current) {
        auto ss =
            reinterpret_cast<sockaddr_storage*>(current->Address.lpSockaddr);
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
    TCHAR buf[256];
    DWORD size = _countof(buf);
    bool result =
        GetComputerNameEx(ComputerNamePhysicalDnsHostname, buf, &size);
    if(!result) {
      LOG_WARNING("Cant retrieve hostname");
      return "windows-pc";
    }

    std::string s(buf);
    return s;
  }

  bool setSelfHostname(const std::string& newHostname)
  {
    // Not implemented in Windows port.
    // This can be done through SetComputerNameEx()
    // Caveat is that the reboot is required for the change to be picked up
    // My opinion is to not touch the netbios hostname, and just add entry into
    // hosts
    return true;
  }

  void updateHostsFile(const std::map<std::string, IpAddress>& data)
  {
    updateHostsFileInternal(data);
  }

  void notifyReady()
  {
    // Not implemented in Windows port.
  }

  void runScripts(std::string path)
  {
    char* conf_path = std::getenv("PROGRAMDATA");
  std::string full_path(conf_path);
  full_path+="\\husarnet\\";
  full_path+=path;
  std::filesystem::path dir(full_path);
  std::string msg = "checking if valid hooks under path " + full_path;
  LOG(msg.c_str());

  if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
    return;
  }

  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() == ".ps1" && (entry.status().permissions() & std::filesystem::perms::owner_exec) == std::filesystem::perms::owner_exec) {
      std::string command = "powershell.exe -File " + entry.path().string();
      std::system(command.c_str());
    }
  }
  }

  bool checkScriptsExist(std::string path)
  {
  char* conf_path = std::getenv("PROGRAMDATA");
  std::string full_path(conf_path);
  full_path+="\\husarnet\\";
  full_path+=path;
  std::filesystem::path dir(full_path);
  std::string msg = "checking if valid hooks under path " + full_path;
  LOG(msg.c_str());

  if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
    return false;
  }

  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() == ".ps1" && (entry.status().permissions() & std::filesystem::perms::owner_exec) == std::filesystem::perms::owner_exec) {
      return true;
    }
  }

  return false;
  }
}  // namespace Privileged
