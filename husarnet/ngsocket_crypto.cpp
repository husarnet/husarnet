// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "ngsocket_crypto.h"
#include "identity.h"
#include "sodium.h"
#include "util.h"

namespace NgSocketCrypto {
std::string makeMessage(const std::string& data, const std::string& kind) {
  return kind + "\n" + data;
}

fstring<64> sign(const std::string& data,
                 const std::string& kind,
                 Identity* identity) {
  std::string msg = makeMessage(data, kind);
  return identity->sign(msg);
}

fstring<16> pubkeyToDeviceId(fstring<32> pubkey) {
  fstring<32> hash;
  crypto_hash_sha256((unsigned char*)&hash[0],
                     (const unsigned char*)pubkey.data(), pubkey.size());
  if (!(hash[0] == 0 && ((unsigned char)hash[1]) < 50)) {
    return BadDeviceId;
  }
  return "\xfc\x94" + std::string(hash).substr(3, 14);
}

bool verifySignature(const std::string& data,
                     const std::string& kind,
                     const fstring<32>& pubkey,
                     const fstring<64>& sig) {
  std::string msg = makeMessage(data, kind);
  int ret = crypto_sign_ed25519_verify_detached(
      (unsigned char*)&sig[0], (const unsigned char*)msg.data(), msg.size(),
      (const unsigned char*)pubkey.data());
  return ret == 0;
}

bool safeEquals(std::string a, std::string b) {
  if (a.size() != b.size())
    return false;

  return sodium_memcmp(a.data(), b.data(), b.size()) == 0;
}

}  // namespace NgSocketCrypto
