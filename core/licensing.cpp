// port.h has to be the first include, newline afterwards prevents autoformatter
// from sorting it
#include "ports/port.h"

#include <sodium.h>
#include <algorithm>
#include <fstream>
#include "filestorage.h"
#include "husarnet_config.h"
#include "licensing.h"
#include "sockets.h"
#include "util.h"

json retrieveLicenseJson(std::string dashboardHostname) {
  IpAddress ip = OsSocket::resolveToIp(dashboardHostname);
  InetAddress address{ip, 80};
  int sockfd = OsSocket::connectTcpSocket(address);
  if (sockfd < 0) {
    exit(1);
  }

  std::string readBuffer;
  readBuffer.resize(8192);

  std::string request =
      "GET /license.json HTTP/1.1\n"
      "Host: app.husarnet.com\n"
      "User-Agent: husarnet\n"
      "Accept: */*\n\n";

  SOCKFUNC(send)(sockfd, request.data(), request.size(), 0);
  size_t len =
      SOCKFUNC(recv)(sockfd, (char*)readBuffer.data(), readBuffer.size(), 0);
  size_t pos = readBuffer.find("\r\n\r\n");

  if (pos == std::string::npos) {
    LOG("invalid response from the server: %s", readBuffer.c_str());
    abort();
  }
  pos += 4;
  return readBuffer.substr(pos, len - pos);

  // FileStorage::writeFile(FileStorage::licenseFilePath(), res->body);  // save
  // to cache, retrieve if online is unavailable
}

static const unsigned char* PUBLIC_KEY = reinterpret_cast<const unsigned char*>(
    "\x2a\x3f\x26\x7c\x2a\x68\xa6\x0f\x66\xf6\xaf\x2b\x0a\x42\x7b\x25"
    "\xb5\x30\x7c\x23\x47\x80\x2d\xdf\x35\x24\xf4\x9a\xfe\x7d\x01\xe5");

static std::string getSignatureData(const json licenseJson) {
  std::string s;
  s.append("1\n");
  s.append(licenseJson["installation_id"].get<std::string>() + "\n");
  s.append(licenseJson["license_id"].get<std::string>() + "\n");
  s.append(licenseJson["name"].get<std::string>() + "\n");
  s.append(licenseJson["max_devices"].get<std::string>() + "\n");
  s.append(licenseJson["dashboard_url"].get<std::string>() + "\n");
  s.append(licenseJson["websetup_host"].get<std::string>() + "\n");
  for (auto address : licenseJson["base_server_addresses"]) {
    s.append(address.get<std::string>() + ",");
  }
  s[s.size() - 1] = '\n';  // remove the last comma
  s.append(licenseJson["issued"].get<std::string>() + "\n");
  s.append(licenseJson["valid_until"].get<std::string>());
  return s;
}

static void verifySignature(const std::string& signature,
                            const std::string& data) {
  auto* signaturePtr = reinterpret_cast<const unsigned char*>(signature.data());
  auto* dataPtr = reinterpret_cast<const unsigned char*>(data.data());
  if (crypto_sign_ed25519_verify_detached(signaturePtr, dataPtr, data.size(),
                                          PUBLIC_KEY) != 0) {
    LOG("license file is invalid");
    abort();
  }
}

License::License(std::string dashboardHostname) {
  auto licenseJson = retrieveLicenseJson(dashboardHostname);
  verifySignature(licenseJson["signature"], getSignatureData(licenseJson));

  this->dashboardUrl = licenseJson["dashboard_url"];
  this->websetupAddress = IpAddress::parse(licenseJson["websetup_address"]);

  for (auto baseAddress : licenseJson["base_server_addresses"]) {
    this->baseServerAddresses.emplace_back(IpAddress::parse(baseAddress));
  }
}

std::string License::getDashboardUrl() {
  return this->dashboardUrl;
}

IpAddress License::getWebsetupAddress() {
  return this->websetupAddress;
}

std::list<IpAddress> License::getBaseServerAddresses() {
  return this->baseServerAddresses;
}
