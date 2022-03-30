// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "identity.h"
#include "sodium.h"

fstring<64> StdIdentity::sign(const std::string& msg) {
  fstring<64> sig;
  unsigned long long siglen = 64;
  int ret = crypto_sign_ed25519_detached(
      (unsigned char*)&sig[0], &siglen, (const unsigned char*)msg.data(),
      msg.size(), (const unsigned char*)privkey.data());
  assert(ret == 0 && siglen == 64);
  return sig;
}
