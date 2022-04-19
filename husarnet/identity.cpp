// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "identity.h"
#include "ngsocket_crypto.h"
#include "sodium.h"

Identity::Identity()
{
  deviceId = BadDeviceId;
}

fstring<32> Identity::getPubkey()
{
  return pubkey;
}

DeviceId Identity::getDeviceId()
{
  return deviceId;
}

fstring<64> Identity::sign(const std::string& msg)
{
  fstring<64> sig;
  unsigned long long siglen = 64;
  int ret = crypto_sign_ed25519_detached(
      (unsigned char*)&sig[0], &siglen, (const unsigned char*)msg.data(),
      msg.size(), (const unsigned char*)privkey.data());
  assert(ret == 0 && siglen == 64);
  return sig;
}

bool Identity::isValid()
{
  if(deviceId == BadDeviceId)
    return false;

  // More tests to come I guess

  return true;
}

Identity Identity::create()
{
  Identity identity;

  while(identity.deviceId == BadDeviceId) {
    crypto_sign_ed25519_keypair(
        (unsigned char*)&identity.pubkey[0],
        (unsigned char*)&identity.privkey[0]);
    identity.deviceId = NgSocketCrypto::pubkeyToDeviceId(identity.pubkey);
  }

  return identity;
}

std::string Identity::serialize()
{
  // TODO implement this
  return "";
}
Identity Identity::deserialize(std::string data)
{
  // TODO implement this
  return Identity();
}
