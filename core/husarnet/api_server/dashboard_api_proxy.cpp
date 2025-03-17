// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/api_server/dashboard_api_proxy.h"

#include "husarnet/logging.h"

void DashboardApiProxy::signAndForward(const httplib::Request& req, httplib::Response& res, const std::string& path)
{
  auto method = req.method;
  LOG_INFO("Forwarding request %s %s", method.c_str(), path.c_str());

  auto apiAddress = configManager->getApiAddress();
  if(!apiAddress.isFC94()) {
    LOG_WARNING(
        "Not forwarding the request, as proxy does not have valid "
        "Dashboard API address");

    // construct response of the same shape as Dashboard API uses
    nlohmann::json jsonResponse{
        {"type", "server_error"},
        {"errors", nlohmann::json::array({"request received by the daemon, but dashboard API "
                                          "address is not known"})},
        {"warnings", nlohmann::json::array()},
        {"message", "error"}};

    res.set_content(jsonResponse.dump(4), "application/json");
    return;
  }

  httplib::Params params;
  params.emplace("pk", httplib::detail::base64_encode(myIdentity->getPubkey()));
  params.emplace("sig", httplib::detail::base64_encode(myIdentity->sign(req.body)));

  std::string query = httplib::detail::params_to_query_str(params);
  std::string pathWithQuery(path + "?" + query);

  httplib::Client httpClient(apiAddress.toString());
  httplib::Result result;

  if(method == "GET") {
    result = httpClient.Get(pathWithQuery);
  } else if(method == "POST") {
    result = httpClient.Post(pathWithQuery, req.body, "application/json");
  } else if(method == "PUT") {
    result = httpClient.Put(pathWithQuery, req.body, "application/json");
  } else if(method == "DELETE") {
    result = httpClient.Delete(pathWithQuery);
  } else {
    LOG_ERROR("Unsupported HTTP method: %s", method.c_str());
    return;
  }

  if(result) {
    res.set_content(result->body,
                    "application/json");  // Dashboard API always returns valid JSON
  } else {
    auto err = result.error();
    LOG_ERROR("Can't contact Dashboard API: %s", httplib::to_string(err).c_str());
  }
}
