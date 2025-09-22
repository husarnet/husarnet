// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <string>

#include <etl/string.h>

#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"

#include "nlohmann/json.hpp"

namespace dashboardapi {
  class Response {
    int statusCode = -1;
    nlohmann::json jsonDoc;

   public:
    Response(int code, const std::string& bytes);
    bool isSuccessful() const;
    nlohmann::json& getPayloadJson();

    std::string toString();
  };

  Response getConfig(HusarnetAddress apiAddress);
  Response postHeartbeat(HusarnetAddress apiAddress, Identity* identity);
  Response postClaim(
      HusarnetAddress apiAddress,
      Identity* identity,
      const etl::string_view& code,
      const etl::string_view& hostname);
}  // namespace dashboardapi
