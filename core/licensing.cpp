// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/licensing.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#include <sodium.h>

#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/husarnet_config.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "ports/port.h"

json retrieveLicenseJson(std::string dashboardHostname)
{
  IpAddress ip = Port::resolveToIp(dashboardHostname);
  InetAddress address{ip, 80};
  int sockfd = OsSocket::connectTcpSocket(address);
  if(sockfd < 0) {
    LOG("Can't contact %s - is DNS resolution working properly?",
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
      "User-Agent: husarnet\n"
      "Accept: */*\n\n";

  SOCKFUNC(send)(sockfd, request.data(), request.size(), 0);
  size_t len =
      SOCKFUNC(recv)(sockfd, (char*)readBuffer.data(), readBuffer.size(), 0);
  size_t pos = readBuffer.find("\r\n\r\n");

  if(pos == std::string::npos) {
    LOG("invalid response from the server: %s", readBuffer.c_str());
    abort();
  }
  pos += 4;
  return json::parse(readBuffer.substr(pos, len - pos));
}

json retrieveCachedLicenseJson()
{
  return json::parse(Privileged::readLicenseJson());
}

static const unsigned char* PUBLIC_KEY = reinterpret_cast<const unsigned char*>(
    "\x2a\x3f\x26\x7c\x2a\x68\xa6\x0f\x66\xf6\xaf\x2b\x0a\x42\x7b\x25"
    "\xb5\x30\x7c\x23\x47\x80\x2d\xdf\x35\x24\xf4\x9a\xfe\x7d\x01\xe5");

#define LICENSE_INSTALLATION_ID_KEY "installation_id"
#define LICENSE_LICENSE_ID_KEY "license_id"
#define LICENSE_NAME_KEY "name"
#define LICENSE_MAX_DEVICES_KEY "max_devices"
#define LICENSE_DASHBOARD_URL_KEY "dashboard_url"
#define LICENSE_WEBSETUP_HOST_KEY "websetup_host"
#define LICENSE_BASE_SERVER_ADDRESSES_KEY "base_server_addresses"
#define LICENSE_ISSUED_KEY "issued"
#define LICENSE_VALID_UNTIL_KEY "valid_until"
#define LICENSE_SIGNATURE_KEY "signature"

static std::string getSignatureData(const json licenseJson)
{
  std::string s;
  s.append("1\n");
  s.append(licenseJson[LICENSE_INSTALLATION_ID_KEY].get<std::string>() + "\n");
  s.append(licenseJson[LICENSE_LICENSE_ID_KEY].get<std::string>() + "\n");
  s.append(licenseJson[LICENSE_NAME_KEY].get<std::string>() + "\n");
  s.append(
      std::to_string(licenseJson[LICENSE_MAX_DEVICES_KEY].get<int>()) + "\n");
  s.append(licenseJson[LICENSE_DASHBOARD_URL_KEY].get<std::string>() + "\n");
  s.append(licenseJson[LICENSE_WEBSETUP_HOST_KEY].get<std::string>() + "\n");
  for(auto address : licenseJson[LICENSE_BASE_SERVER_ADDRESSES_KEY]) {
    s.append(address.get<std::string>() + ",");
  }
  s[s.size() - 1] = '\n';  // remove the last comma
  s.append(licenseJson[LICENSE_ISSUED_KEY].get<std::string>() + "\n");
  s.append(licenseJson[LICENSE_VALID_UNTIL_KEY].get<std::string>());
  return s;
}

static void verifySignature(
    const std::string& signature,
    const std::string& data)
{
  auto* signaturePtr = reinterpret_cast<const unsigned char*>(signature.data());
  auto* dataPtr = reinterpret_cast<const unsigned char*>(data.data());
  if(crypto_sign_ed25519_verify_detached(
         signaturePtr, dataPtr, data.size(), PUBLIC_KEY) != 0) {
    LOG("license file is invalid");
    abort();
  }
}

License::License(std::string dashboardHostname)
{
  auto licenseJson = retrieveLicenseJson(dashboardHostname);

  if(licenseJson.empty()) {
    licenseJson = retrieveCachedLicenseJson();

    if(licenseJson.empty()) {
      LOG("No license! Husarnet Daemon can't start without license.json. "
          "License not found on local disk and download was not possible. "
          "Exiting.");
      abort();
    }

    LOG("Found cached license.json on local disk, proceed");
  }

  verifySignature(
      base64Decode(licenseJson[LICENSE_SIGNATURE_KEY].get<std::string>()),
      getSignatureData(licenseJson));

  this->dashboardFqdn =
      licenseJson[LICENSE_DASHBOARD_URL_KEY].get<std::string>();
  this->websetupAddress = IpAddress::parse(
      licenseJson[LICENSE_WEBSETUP_HOST_KEY].get<std::string>());

  for(auto baseAddress : licenseJson[LICENSE_BASE_SERVER_ADDRESSES_KEY]) {
    this->baseServerAddresses.emplace_back(
        IpAddress::parse(baseAddress.get<std::string>()));
  }

  Privileged::writeLicenseJson(licenseJson.dump());
}

std::string License::getDashboardFqdn()
{
  if(this->dashboardFqdn.starts_with("https://")) {
    return this->dashboardFqdn.substr(strlen("https://"));
  }

  if(this->dashboardFqdn.starts_with("http://")) {
    return this->dashboardFqdn.substr(strlen("http://"));
  }

  return this->dashboardFqdn;
}

IpAddress License::getWebsetupAddress()
{
  return this->websetupAddress;
}

std::vector<IpAddress> License::getBaseServerAddresses()
{
  return this->baseServerAddresses;
}
