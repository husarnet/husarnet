// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/config_manager.h"
#include "husarnet/identity.h"

#include "nlohmann/json.hpp"

namespace dashboardapi {
  constexpr int base64EncodedPublicKeySize = 44;
  constexpr int base64EncodedSignatureSize = 88;

  class Proxy {
   public:
    static etl::string<base64EncodedPublicKeySize> encodePublicKey(Identity* identity);
    static etl::string<base64EncodedSignatureSize> encodeSignature(Identity* identity, const std::string& body);
  };
}  // namespace dashboardapi
