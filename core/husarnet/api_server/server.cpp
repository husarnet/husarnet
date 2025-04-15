// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/api_server/server.h"

#include <list>
#include <map>
#include <utility>

#include <stdlib.h>

#include "husarnet/config_manager.h"
#include "husarnet/dashboardapi/proxy.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/peer.h"
#include "husarnet/peer_container.h"
#include "husarnet/util.h"

#include "httplib.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

ApiServer::ApiServer(
    ConfigEnv* configEnv,
    ConfigManager* configManager,
    HusarnetManager* husarnetManager,
    Identity* identity)
    : configEnv(configEnv), configManager(configManager), husarnetManager(husarnetManager), identity(identity)
{
}

void ApiServer::returnSuccess(const httplib::Request& req, httplib::Response& res, json result)
{
  json doc;

  doc["status"] = "success";
  doc["result"] = result;

  res.set_content(doc.dump(4), "text/json");
}

void ApiServer::returnSuccess(const httplib::Request& req, httplib::Response& res)
{
  returnSuccess(req, res, "success");
}

void ApiServer::returnInvalidQuery(const httplib::Request& req, httplib::Response& res, std::string errorString)
{
  LOG_ERROR("httpApi: returning invalid query: %s", errorString.c_str());
  json doc;

  doc["status"] = "invalid";
  doc["error"] = errorString;

  res.set_content(doc.dump(4), "text/json");
}

void ApiServer::returnError(const httplib::Request& req, httplib::Response& res, std::string errorString)
{
  LOG_ERROR("httpApi: returning error: %s", errorString.c_str());
  json doc;

  doc["status"] = "error";
  doc["error"] = errorString;

  res.set_content(doc.dump(4), "text/json");
}

static const std::string getOrCreateDaemonApiToken()
{
  auto token = Port::readStorage(StorageKey::daemonApiToken);
  if(token.empty()) {
    auto newToken = generateRandomString(32);
    auto success = Port::writeStorage(StorageKey::daemonApiToken, newToken);
    if(!success) {
      LOG_CRITICAL(
          "Failed to write daemon API token to storage, you won't be able to "
          "use CLI (or daemon's API) to control the daemon!");
    }
    return newToken;
  }
  return token;
}

bool ApiServer::validateSecret(const httplib::Request& req, httplib::Response& res)
{
  if(!req.has_param("secret") || req.get_param_value("secret") != this->daemonApiToken) {
    returnInvalidQuery(req, res, "invalid control secret");
    return false;
  }

  return true;
}

bool ApiServer::requireParams(const httplib::Request& req, httplib::Response& res, std::list<std::string> paramNames)
{
  for(auto paramName : paramNames) {
    if(!req.has_param(paramName.c_str())) {
      returnInvalidQuery(req, res, "missing param " + paramName);
      return false;
    }
  }
  return true;
}

void ApiServer::forwardRequestToDashboardApi(const httplib::Request& req, httplib::Response& res)
{
  if(!validateSecret(req, res)) {
    return;
  }

  std::string route = req.matches[1];
  if(route.empty()) {
    returnError(req, res, "Invalid route");
    return;
  }

  auto method = req.method;
  auto path = "/" + route;
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
  auto encodedPK = dashboardapi::Proxy::encodePublicKey(identity);
  auto encodedSig = dashboardapi::Proxy::encodeSignature(identity, req.body);

  params.emplace("pk", encodedPK.c_str());
  params.emplace("sig", encodedSig.c_str());

  std::string query = httplib::detail::params_to_query_str(params);
  std::string pathWithQuery(path + "?" + query);

  httplib::Client httpClient(apiAddress.toString());
  httpClient.set_connection_timeout(0, 5 * 1000000); // 5 sec
  httpClient.set_read_timeout(5, 0); // 5 seconds
  httpClient.set_write_timeout(5, 0); // 5 seconds
  // TODO: add retries and figure out if timeout values are sane/useful

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
    res.status = 502;
  }
}

template <typename iterable_InetAddress_t>
std::list<std::string> stringifyInetAddressList(const iterable_InetAddress_t& source)
{
  std::list<std::string> stringified{};
  std::transform(source.cbegin(), source.cend(), std::back_insert_iterator(stringified), [](const InetAddress element) {
    return element.str();
  });
  return stringified;
}

template <typename iterable_IpAddress_t>
std::list<std::string> stringifyIpAddressList(const iterable_IpAddress_t& source)
{
  std::list<std::string> stringified{};
  std::transform(source.cbegin(), source.cend(), std::back_insert_iterator(stringified), [](const IpAddress element) {
    return element.toString();
  });
  return stringified;
}

void ApiServer::runThread()
{
  std::unique_lock<std::mutex> lk(mutex);
  httplib::Server svr;

  this->daemonApiToken = getOrCreateDaemonApiToken();
  if(this->daemonApiToken.empty()) {
    LOG_ERROR("was unable to generate daemon api token");
  }

  // Test endpoint
  svr.Get(
      "/hi", [](const httplib::Request&, httplib::Response& res) { res.set_content("Hello World!", "text/plain"); });

  svr.Get("/api/status", [&](const httplib::Request& req, httplib::Response& res) {
    returnSuccess(
        req, res,
        json::object({
            {"config", this->configManager->getDataForStatus()},
            {"live", this->husarnetManager->getDataForStatus()},
            {"version", HUSARNET_VERSION},
            {"user_agent", HUSARNET_USER_AGENT},
        }));
  });

  svr.Post("/api/whitelist/add", [&](const httplib::Request& req, httplib::Response& res) {
    if(!validateSecret(req, res)) {
      return;
    }

    if(!requireParams(req, res, {"address"})) {
      return;
    }

    if(configManager->userWhitelistAdd(IpAddress::parse(req.get_param_value("address")))) {
      returnSuccess(req, res, {});
    } else {
      returnError(req, res, "Failed to add address to whitelist");
    }
  });

  svr.Post("/api/whitelist/rm", [&](const httplib::Request& req, httplib::Response& res) {
    if(!validateSecret(req, res)) {
      return;
    }

    if(!requireParams(req, res, {"address"})) {
      return;
    }

    if(configManager->userWhitelistRm(IpAddress::parse(req.get_param_value("address")))) {
      returnSuccess(req, res, {});
    } else {
      returnError(req, res, "Failed to remove address from whitelist");
    }
  });

  svr.Get(R"(/api/forward/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
    forwardRequestToDashboardApi(req, res);
  });

  svr.Post(R"(/api/forward/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
    forwardRequestToDashboardApi(req, res);
  });

  svr.Put(R"(/api/forward/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
    forwardRequestToDashboardApi(req, res);
  });

  svr.Delete(R"(/api/forward/(.*))", [&](const httplib::Request& req, httplib::Response& res) {
    forwardRequestToDashboardApi(req, res);
  });

  IpAddress bindAddress{};

  if(configEnv->getDaemonApiInterface() != "") {
    // Deduce bind address from the provided interface
    bindAddress = Port::getIpAddressFromInterfaceName(configEnv->getDaemonApiInterface());
    LOG_INFO(
        "Deducing bind address %s from the provided interface: %s", bindAddress.toString().c_str(),
        configEnv->getDaemonApiInterface().c_str());
  } else {
    // Use provided/default bind address
    bindAddress = configEnv->getDaemonApiHost();
  }

  if(!svr.bind_to_port(bindAddress.toString().c_str(), configEnv->getDaemonApiPort())) {
    LOG_CRITICAL(
        "Unable to bind HTTP thread to port %s:%d. Exiting!", bindAddress.toString().c_str(),
        configEnv->getDaemonApiPort());
    exit(1);
  } else {
    LOG_INFO(
        "HTTP thread bound to %s:%d. Will start handling the "
        "connections.",
        bindAddress.toString().c_str(), configEnv->getDaemonApiPort());
  }

  isReady = true;
  lk.unlock();
  cv.notify_all();

  if(!svr.listen_after_bind()) {
    LOG_CRITICAL("HTTP thread finished unexpectedly. Exiting!");
    exit(1);
  }
}

void ApiServer::waitStarted()
{
  std::unique_lock<std::mutex> lk(mutex);
  while(!isReady) {
    cv.wait(lk);
  }
}
