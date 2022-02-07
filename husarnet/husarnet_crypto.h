// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)
#pragma once
#include <string>
#include "device_id.h"
#include "fstring.h"

struct Identity {
  fstring<32> pubkey;
  DeviceId deviceId;

  virtual fstring<64> sign(const std::string& data) = 0;
};

struct StdIdentity : Identity {
  fstring<64> privkey;
  fstring<64> sign(const std::string& data) override;
};

namespace NgSocketCrypto {
std::string makeMessage(const std::string& data, const std::string& kind);
fstring<64> sign(const std::string& data,
                 const std::string& kind,
                 Identity* identity);
fstring<16> pubkeyToDeviceId(fstring<32> pubkey);
bool verifySignature(const std::string& data,
                     const std::string& kind,
                     const fstring<32>& pubkey,
                     const fstring<64>& sig);
std::pair<fstring<32>, fstring<64> > generateId();
bool safeEquals(std::string a, std::string b);
}  // namespace NgSocketCrypto
