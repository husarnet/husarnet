// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include "device_id.h"
#include "fstring.h"
#include "identity.h"

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
bool safeEquals(std::string a, std::string b);
}  // namespace NgSocketCrypto
