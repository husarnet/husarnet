// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/identity.h"

#include <sstream>

#include "husarnet/ports/port_interface.h"

#include "husarnet/logging.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/util.h"

#include "sodium.h"

Identity::Identity()
{
  deviceId = BadDeviceId;
}

bool Identity::isValid()
{
  if(deviceId == BadDeviceId)
    return false;

  // More tests to come I guess

  return true;
}

fstring<32> Identity::getPubkey()
{
  return this->pubkey;
}

DeviceId Identity::getDeviceId()
{
  return this->deviceId;
}

IpAddress Identity::getIpAddress()
{
  return deviceIdToIpAddress(this->getDeviceId());
}

fstring<64> Identity::sign(const std::string& data)
{
  fstring<64> sig;
  unsigned long long siglen = 64;
  crypto_sign_ed25519_detached(
      (unsigned char*)&sig[0], &siglen, (const unsigned char*)data.data(),
      data.size(), (const unsigned char*)privkey.data());
  return sig;
}

Identity* Identity::create()
{
  auto identity = new Identity();

  while(identity->deviceId == BadDeviceId) {
    crypto_sign_ed25519_keypair(
        (unsigned char*)&identity->pubkey[0],
        (unsigned char*)&identity->privkey[0]);
    identity->deviceId = NgSocketCrypto::pubkeyToDeviceId(identity->pubkey);
  }

  return identity;
}

std::string Identity::serialize()
{
  std::stringstream buffer;

  buffer << deviceIdToIpAddress(NgSocketCrypto::pubkeyToDeviceId(pubkey)).str();
  buffer << " ";

  buffer << encodeHex(pubkey);
  buffer << " ";

  buffer << encodeHex(privkey);
  buffer << std::endl;

  return buffer.str();
}

Identity* Identity::deserialize(const std::string& data)
{
  std::stringstream buffer;
  auto identity = new Identity();

  if(data.empty()) {
    return identity;
  }

  buffer << data;

  std::string ipStr, pubkeyStr, privkeyStr;
  buffer >> ipStr >> pubkeyStr >> privkeyStr;

  identity->pubkey = decodeHex(pubkeyStr);
  identity->privkey = decodeHex(privkeyStr);
  identity->deviceId = IpAddress::parse(ipStr.c_str()).toBinary();

  return identity;
}

Identity* Identity::init()
{
  Identity* identity = new Identity();

  std::string id_string = Port::readStorage(StorageKey::id);
  if(id_string.empty()) {
    LOG_WARNING("No identity found!");
  } else {
    identity = Identity::deserialize(id_string);
  }

  if(!identity->isValid()) {
    LOG_ERROR("Identity is invalid, generating a new one");

    identity = Identity::create();

    auto success = Port::writeStorage(StorageKey::id, identity->serialize());
    if(!success) {
      LOG_CRITICAL(
          "Failed to save identity to storage, will run with a volatile one!");
    }
  }

  return identity;
}
