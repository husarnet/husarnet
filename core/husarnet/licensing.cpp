// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/licensing.h"

#include <sodium.h>

#include "husarnet/logging.h"
#include "husarnet/util.h"

static const unsigned char* const PUBLIC_KEY[] = {
    reinterpret_cast<const unsigned char*>(
        "\x2a\x3f\x26\x7c\x2a\x68\xa6\x0f\x66\xf6\xaf\x2b\x0a\x42\x7b\x25"
        "\xb5\x30\x7c\x23\x47\x80\x2d\xdf\x35\x24\xf4\x9a\xfe\x7d\x01\xe5"),
    reinterpret_cast<const unsigned char*>(
        "\x52\x67\x82\x64\xe6\xa8\xc7\xa8\x8c\xdd\x43\x4f\xc7\x77\x60\xe6"
        "\xc5\x67\x75\xe7\x35\x2a\x83\xfe\x4f\x7f\xf7\x53\x2a\x5e\x6a\x34"),
    reinterpret_cast<const unsigned char*>(
        "\xd7\xf1\x42\x73\x36\xf2\xfc\xd8\x2c\xf4\x45\x27\xbb\x14\xe4\x55"
        "\x31\xe5\x5e\x07\xd5\xa3\xb6\x38\x95\x75\xa5\x5c\xba\x28\xd6\x09"),
    reinterpret_cast<const unsigned char*>(
        "\x8d\xeb\xaa\xb5\x41\x8e\x73\x4a\x01\x94\x72\xf0\x64\x5f\x14\x32"
        "\x87\xe0\xed\x54\xc8\xe0\x62\x9c\xfd\xea\x30\x6e\x8b\xb5\x9c\x9d"),
    reinterpret_cast<const unsigned char*>(
        "\x02\x5e\xb8\xb0\xb0\x51\xcf\xe7\x7e\x9f\x50\x4f\x43\x65\xcf\x10"
        "\x14\xc1\xb1\xa3\xd5\x6d\xa2\x15\x5a\x98\xc4\x27\x45\x4e\xea\xa8"),
    reinterpret_cast<const unsigned char*>(
        "\x1d\x9b\x46\x36\x4a\x10\x45\x7a\x22\xb2\xaf\x2f\x9d\xf9\x4b\x03"
        "\x8f\xe3\x01\xfb\x90\x6a\x48\xea\x26\x2e\xe6\xa4\x89\x79\x27\xf4"),
};
const int PUBLIC_KEY_COUNT = sizeof(PUBLIC_KEY) / sizeof(PUBLIC_KEY[0]);

#define LICENSE_VERSION_KEY "version"
#define LICENSE_INSTALLATION_ID_KEY "installation_id"
#define LICENSE_LICENSE_ID_KEY "license_id"
#define LICENSE_NAME_KEY "name"
#define LICENSE_MAX_DEVICES_KEY "max_devices"
#define LICENSE_DASHBOARD_URL_KEY "dashboard_url"
#define LICENSE_WEBSETUP_HOST_KEY "websetup_host"
#define LICENSE_BASE_SERVER_ADDRESSES_KEY "base_server_addresses"
#define LICENSE_ISSUED_KEY "issued"
#define LICENSE_VALID_UNTIL_KEY "valid_until"
#define LICENSE_API_SERVERS_KEY "api_servers"
#define LICENSE_EB_SERVERS_KEY "eb_servers"
#define LICENSE_SIGNATURE_KEY "signature"
#define LICENSE_SIGNATURE_V2_KEY "signature_v2"

static std::string generateSignatureData(const json& licenseJson)
{
  std::string s;
  s.append(std::to_string(licenseJson[LICENSE_VERSION_KEY].get<int>()) + "\n");
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

  for(auto address : licenseJson[LICENSE_API_SERVERS_KEY]) {
    s.append(address.get<std::string>() + ",");
  }
  s[s.size() - 1] = '\n';

  for(auto address : licenseJson[LICENSE_EB_SERVERS_KEY]) {
    s.append(address.get<std::string>() + ",");
  }
  s[s.size() - 1] = '\n';

  s.append(licenseJson[LICENSE_ISSUED_KEY].get<std::string>() + "\n");
  s.append(licenseJson[LICENSE_VALID_UNTIL_KEY].get<std::string>());
  return s;
}

static bool isJsonValid(const json licenseJson)
{
  if(licenseJson.empty() || licenseJson.is_discarded()) {
    LOG_CRITICAL("license data is invalid - malformed or empty JSON");
    return false;
  }

  // These fields are actually needed as the metadata
  if(!licenseJson.contains(LICENSE_DASHBOARD_URL_KEY) ||
     !licenseJson.contains(LICENSE_BASE_SERVER_ADDRESSES_KEY) ||
     !licenseJson.contains(LICENSE_API_SERVERS_KEY) ||
     !licenseJson.contains(LICENSE_EB_SERVERS_KEY)) {
    LOG_CRITICAL("license data is invalid - missing required fields");
    return false;
  }

  return true;
}

static bool verifySignature(const json& licenseJson)
{
  // This is mostly for checking if a minimum license version is present
  if(!licenseJson.contains(LICENSE_SIGNATURE_V2_KEY)) {
    LOG_CRITICAL(
        "license data is invalid - missing signature for the required license "
        "version");
    return false;
  }

  auto declaredSignature =
      base64Decode(licenseJson[LICENSE_SIGNATURE_V2_KEY].get<std::string>());
  auto signatureData = generateSignatureData(licenseJson);

  for(int i = 0; i < PUBLIC_KEY_COUNT; i++) {
    if(crypto_sign_ed25519_verify_detached(
           reinterpret_cast<const unsigned char*>(declaredSignature.c_str()),
           reinterpret_cast<const unsigned char*>(signatureData.c_str()),
           signatureData.size(), PUBLIC_KEY[i]) == 0) {
      return true;
    }
  }

  return false;
}

bool isLicenseValid(const json& licenseJson)
{
  if(!isJsonValid(licenseJson)) {
    return false;
  }

  if(!verifySignature(licenseJson)) {
    return false;
  }

  return true;
}
