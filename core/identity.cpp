// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/identity.h"

#include <sstream>

#include "husarnet/logging.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/util.h"

#include "sodium.h"

Identity::Identity()
{
  peerId = BadPeerId;
}

fstring<32> Identity::getPubkey()
{
  return pubkey;
}

PeerId Identity::getPeerId()
{
  return peerId;
}

IpAddress Identity::getIpAddress()
{
  return peerIdToIpAddress(getPeerId());
}

fstring<64> Identity::sign(const std::string& msg)
{
  fstring<64> sig;
  unsigned long long siglen = 64;
  crypto_sign_ed25519_detached(
      (unsigned char*)&sig[0], &siglen, (const unsigned char*)msg.data(),
      msg.size(), (const unsigned char*)privkey.data());
  return sig;
}

bool Identity::isValid()
{
  if(peerId == BadPeerId)
    return false;

  // More tests to come I guess

  return true;
}

Identity Identity::create()
{
  Identity identity;

  while(identity.peerId == BadPeerId) {
    crypto_sign_ed25519_keypair(
        (unsigned char*)&identity.pubkey[0],
        (unsigned char*)&identity.privkey[0]);
    identity.peerId = NgSocketCrypto::pubkeyToPeerId(identity.pubkey);
  }

  return identity;
}

std::string Identity::serialize()
{
  std::stringstream buffer;

  buffer << peerIdToIpAddress(NgSocketCrypto::pubkeyToPeerId(pubkey)).str();
  buffer << " ";

  buffer << encodeHex(pubkey);
  buffer << " ";

  buffer << encodeHex(privkey);
  buffer << std::endl;

  return buffer.str();
}

Identity Identity::deserialize(std::string data)
{
  std::stringstream buffer;
  buffer << data;

  std::string ipStr, pubkeyStr, privkeyStr;
  buffer >> ipStr >> pubkeyStr >> privkeyStr;

  auto identity = new Identity();
  identity->pubkey = decodeHex(pubkeyStr);
  identity->privkey = decodeHex(privkeyStr);
  identity->peerId = IpAddress::parse(ipStr.c_str()).toBinary();
  return *identity;
}
