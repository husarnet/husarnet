// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/api_server/dashboard_api_proxy.h"

#include "husarnet/logging.h"

bool DashboardApiProxy::isValid()
{
  return !apiAddress.empty();
}

void DashboardApiProxy::signAndForward(
    const httplib::Request& req,
    httplib::Response& res,
    const std::string& path)
{
  nlohmann::json payload = nlohmann::json::parse(req.body, nullptr, false);
  if(payload.is_discarded()) {
    LOG_INFO("HTTP request body is not a valid JSON");
    return;
  }

  auto pk = httplib::detail::base64_encode(identity->getPubkey());
  auto sig = httplib::detail::base64_encode(identity->sign(payload.dump()));
  nlohmann::json signedBody = {{"pk", pk}, {"sig", sig}, {"payload", payload}};

  if(auto result =
         httpClient.Post(path, signedBody.dump(), "application/json")) {
    res.set_content(
        result->body,
        "application/json");  // Dashboard API always returns valid JSON
  } else {
    auto err = result.error();
    LOG_ERROR(
        "Can't contact Dashboard API: %s", httplib::to_string(err).c_str());
  }
}