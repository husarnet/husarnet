// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

#include "response.h"

#include "husarnet/husarnet_config.h"

#include "etl/string.h"
#include "etl/string_view.h"
#include "port_interface.h"
#include "proxy.h"

namespace dashboardapi {
  Response::Response(int code, const std::string& bytes) : statusCode(code)
  {
    if(code == 200) {
      jsonDoc = nlohmann::json::parse(bytes);
    } else {
      jsonDoc = nlohmann::json::parse("{}");
    }
  }
  bool Response::isSuccessful() const
  {
    if(this->statusCode != 200) {
      return false;
    }
    return jsonDoc["type"].get<std::string>() == "success";
  }

  nlohmann::json& Response::getPayloadJson()
  {
    return this->jsonDoc["payload"];
  }

  std::string Response::toString()
  {
    std::string result{};
    if(jsonDoc["type"] == "user_error") {
      result += "invalid request: ";
    } else if(jsonDoc["type"] == "server_error") {
      result += "server error: ";
    } else if(jsonDoc["type"] == nullptr) {
      result += "unknown error type null";
      return result;
    } else {
      result += "unknown api error: ";
    }

    auto errors = jsonDoc["errors"].get<std::vector<std::string>>();
    for(auto& err : errors) {
      result += err;
    }

    return result;
  }

  Response getConfig(HusarnetAddress apiAddress)
  {
    auto [statusCode, bytes] = Port::httpGet(apiAddress, "/v3/device/get_config");
    return {statusCode, bytes};
  }

  // FIXME:
  //  there _should_ be a compile-time function to calculate those in etl::base64
  //  documentation even mentions such construct, unfortunately I couldn't find it in the lib sources

  // TODO: generalize it
  Response postHeartbeat(HusarnetAddress apiAddress, Identity* identity)
  {
    std::string path("/v3/device/manage/heartbeat");

    std::string body(R"({"user_agent":")");
    body.append(HUSARNET_USER_AGENT);
    body.append("\"}");

    auto encodedPK = Proxy::encodePublicKey(identity);
    auto encodedSig = Proxy::encodeSignature(identity, body);

    // build query string
    path.append("?pk=");
    path.append(encodedPK.begin(), encodedPK.size());
    path.append("&sig=");
    path.append(encodedSig.begin(), encodedSig.size());

    auto [statusCode, bytes] = Port::httpPost(apiAddress, path, body);
    return {statusCode, bytes};
  }

  // Used only for platforms without CLI support (ESP-32).
  // On fat platforms claims are done via the CLI.
  Response postClaim(
      HusarnetAddress apiAddress,
      Identity* identity,
      const etl::string_view& code,
      const etl::string_view& hostname)
  {
    std::string path("/v3/device/manage/claim");

    std::string body(R"({"code":")");
    body.append(code.data(), code.size());
    body.append(R"(","hostname":")");
    body.append(hostname.data(), hostname.size());
    body.append(R"("})");

    auto encodedPK = Proxy::encodePublicKey(identity);
    auto encodedSig = Proxy::encodeSignature(identity, body);

    // build query string
    path.append("?pk=");
    path.append(encodedPK.begin(), encodedPK.size());
    path.append("&sig=");
    path.append(encodedSig.begin(), encodedSig.size());

    auto [statusCode, bytes] = Port::httpPost(apiAddress, path, body);
    return {statusCode, bytes};
  }
}  // namespace dashboardapi
