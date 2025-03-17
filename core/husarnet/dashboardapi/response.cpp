// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

#include "response.h"
#include "port_interface.h"

namespace dashboardapi {
  Response::Response(int code, const std::string& bytes) : statusCode(code) {
  jsonDoc = nlohmann::json::parse(bytes);
}
  bool Response::isSuccessful() const
  {
    if(this->statusCode != 200) {
      return false;
    }
    return jsonDoc["type"].get<std::string>() == "success";
  }

  nlohmann::json& Response::getPayloadJson() {
    return this->jsonDoc["payload"];
  }

  std::string Response::toString() {
    std::string result{};
    if(jsonDoc["type"] == "user_error") {
      result += "invalid request: ";
    } else if(jsonDoc["type"] == "server_error") {
      result += "server error: ";
    } else {
      result += "unknown api error: ";
    }

    auto errors = jsonDoc["errors"].get<std::vector<std::string>>();
    for(auto& err : errors) {
      result += err;
    }

    return result;
  }

  Response getConfig(HusarnetAddress apiAddress) {
    auto [statusCode, bytes] = Port::httpGet(apiAddress.toString(), "/device/get_config");
    Response res(statusCode, bytes);
    return res;
  }
}