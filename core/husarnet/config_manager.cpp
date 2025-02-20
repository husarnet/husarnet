// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/config_manager.h"

/*

std::string ConfigStorage::serialize()
{
#ifdef PORT_ESP32
  return this->currentData.dump(0);
#else
  return this->currentData.dump(4);
#endif
}

void ConfigStorage::deserialize(std::string blob)
{
  // Initialize an empty file with something JSON-ish
  if(blob.length() < 3) {
    blob = "{}";
  }

  this->currentData = json::parse(blob);

  // Make sure that all data is in a proper format
  if(!currentData[HOST_TABLE_KEY].is_object()) {
    currentData[HOST_TABLE_KEY] = json::object();
  }
  if(!currentData[WHITELIST_KEY].is_array()) {
    currentData[WHITELIST_KEY] = json::array();
  }
  if(!currentData[INTERNAL_SETTINGS_KEY].is_object()) {
    currentData[INTERNAL_SETTINGS_KEY] = json::object();
  }
  if(!currentData[USER_SETTINGS_KEY].is_object()) {
    currentData[USER_SETTINGS_KEY] = json::object();
  }
}

void ConfigStorage::save()
{
  if(!this->shouldSaveImmediately) {
    return;
  }

  manager->getHooksManager()->withRw([&]() {
    LOG_DEBUG("saving settings for ConfigStorage");
    writeFunc(serialize());
    updateHostsInSystem();
  });
}

json retrieveLicenseJson(
    std::string dashboardHostname,
    bool abortOnFailure = true)
{
  IpAddress ip = Port::resolveToIp(dashboardHostname);
  InetAddress address{ip, 80};
  int sockfd = OsSocket::connectUnmanagedTcpSocket(address);
  if(sockfd < 0) {
    LOG_ERROR(
        "Can't contact %s - is DNS resolution working properly?",
        dashboardHostname.c_str());
    return json::parse("{}");
  }

  std::string readBuffer;
  readBuffer.resize(8192);

  std::string request =
      "GET /license.json HTTP/1.1\n"
      "Host: " +
      dashboardHostname +
      "\n"
      "User-Agent: husarnet\n"  // TODO change this to HUSARNET_USER_AGENT
      "Accept: *\n\n"; // fix this accept header

  SOCKFUNC(send)(sockfd, request.data(), request.size(), 0);

  // TODO add a while loop here
  size_t len =
      SOCKFUNC(recv)(sockfd, (char*)readBuffer.data(), readBuffer.size(), 0);
  size_t pos = readBuffer.find("\r\n\r\n");

  if(pos == std::string::npos) {
    LOG_ERROR("invalid response from the server: %s", readBuffer.c_str());

    if(abortOnFailure) {
      abort();
    } else {
      return json::parse("{}");
    }
  }
  pos += 4;

  LOG_DEBUG(
      "License retrieved from %s, size %d", dashboardHostname.c_str(), len);

  return json::parse(readBuffer.substr(pos, len - pos), nullptr, false);
}

json retrieveCachedLicenseJson()
{
  // Don't throw an exception on parse failure
  return json::parse(Port::readLicenseJson(), nullptr, false);
}
*/
