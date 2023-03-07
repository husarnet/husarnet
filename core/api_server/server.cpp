// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/api_server/server.h"

#include <list>
#include <map>
#include <utility>

#include <stdlib.h>

#include "husarnet/config_storage.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/logmanager.h"
#include "husarnet/peer.h"
#include "husarnet/peer_container.h"
#include "husarnet/util.h"

#include "httplib.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

ApiServer::ApiServer(HusarnetManager* manager) : manager(manager)
{
  manager->rotateApiSecret();
}

void ApiServer::returnSuccess(
    const httplib::Request& req,
    httplib::Response& res,
    json result)
{
  json doc;

  doc["status"] = "success";
  doc["result"] = result;

  res.set_content(doc.dump(4), "text/json");
}

void ApiServer::returnSuccess(
    const httplib::Request& req,
    httplib::Response& res)
{
  returnSuccess(req, res, "success");
}

void ApiServer::returnInvalidQuery(
    const httplib::Request& req,
    httplib::Response& res,
    std::string errorString)
{
  LOG_ERROR("httpApi: returning invalid query: %s", errorString.c_str());
  json doc;

  doc["status"] = "invalid";
  doc["error"] = errorString;

  res.set_content(doc.dump(4), "text/json");
}

void ApiServer::returnError(
    const httplib::Request& req,
    httplib::Response& res,
    std::string errorString)
{
  LOG_ERROR("httpApi: returning error: %s", errorString.c_str());
  json doc;

  doc["status"] = "error";
  doc["error"] = errorString;

  res.set_content(doc.dump(4), "text/json");
}

bool ApiServer::validateSecret(
    const httplib::Request& req,
    httplib::Response& res)
{
  if(!req.has_param("secret") ||
     req.get_param_value("secret") != manager->getApiSecret()) {
    returnInvalidQuery(req, res, "invalid control secret");
    return false;
  }

  return true;
}

bool ApiServer::requireParams(
    const httplib::Request& req,
    httplib::Response& res,
    std::list<std::string> paramNames)
{
  for(auto paramName : paramNames) {
    if(!req.has_param(paramName.c_str())) {
      returnInvalidQuery(req, res, "missing param " + paramName);
      return false;
    }
  }
  return true;
}

std::list<std::string> stringifyInetAddressList(
    const std::list<InetAddress>& source)
{
  std::list<std::string> stringified;
  std::transform(
      source.cbegin(), source.cend(), stringified.begin(),
      [](const InetAddress element) { return element.str(); });

  return stringified;
}

void ApiServer::runThread()
{
  httplib::Server svr;

  // Test endpoint
  svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("Hello World!", "text/plain");
  });

  svr.Get(
      "/api/status", [&](const httplib::Request& req, httplib::Response& res) {
        auto hostTable = manager->getConfigStorage().getHostTable();
        std::map<std::string, std::string> hostTableStringified;

        for(auto& [hostname, address] : hostTable) {
          hostTableStringified.insert({hostname, address.toString()});
        }

        std::list<std::string> whitelistStringified;
        for(auto& element : manager->getWhitelist()) {
          whitelistStringified.push_back(element.toString());
        }

        std::list<json> peerData;
        for(auto& [peerId, rawPeer] : manager->getPeerContainer()->getPeers()) {
          json newPeer = {
              {"husarnet_address", rawPeer->getIpAddress().str()},
              {"is_active", rawPeer->isActive()},
              {"is_reestablishing", rawPeer->isReestablishing()},
              {"is_tunelled", rawPeer->isTunelled()},
              {"is_secure", rawPeer->isSecure()},
              {"source_addresses",
               stringifyInetAddressList(rawPeer->getSourceAddresses())},
              {"target_addresses",
               stringifyInetAddressList(rawPeer->getTargetAddresses())},
              {"used_target_address", rawPeer->getUsedTargetAddress().str()},
              {"link_local_address", rawPeer->getLinkLocalAddress().str()},
              // {"latency_ms", manager->getLatency(rawPeer->getDeviceId())}, //
              // TODO this is kinda bugged at the moment. Fix this
          };

          peerData.push_back(newPeer);
        }

        returnSuccess(
            req, res,
            json::object({
                {"version", manager->getVersion()},
                {"local_ip", manager->getSelfAddress().toString()},
                {"local_hostname", manager->getSelfHostname()},
                {"is_dirty", manager->isDirty()},
                {"hooks_enabled", manager->areHooksEnabled()},
                {"is_joined", manager->isJoined()},
                {"is_ready_to_join",
                 manager
                     ->isConnectedToBase()},  // base server connection should
                                              // be enough to assume that we can
                                              // try connectting to websetup
                {"is_ready", manager->isConnectedToBase()},
                {"connection_status",
                 {
                     {"base", manager->isConnectedToBase()},
                     {"websetup", manager->isConnectedToWebsetup()},
                 }},
                {"dashboard_fqdn", manager->getDashboardFqdn()},
                {"websetup_address", manager->getWebsetupAddress().toString()},
                {"base_connection",
                 {{"type", manager->getCurrentBaseProtocol()},
                  {"address", manager->getCurrentBaseAddress().ip.toString()},
                  {"port", manager->getCurrentBaseAddress().port}}},
                {"host_table", hostTableStringified},
                {"whitelist", whitelistStringified},
                {"user_settings",
                 manager->getConfigStorage().getUserSettings()},
                {"peers", peerData},
            }));
      });

  svr.Post(
      "/api/join", [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        if(!requireParams(req, res, {"code"})) {
          return;
        }

        std::string code = req.get_param_value("code");
        std::string hostname =
            req.has_param("hostname") ? req.get_param_value("hostname") : "";

        manager->joinNetwork(code, hostname);
        returnSuccess(req, res);
      });

  svr.Post(
      "/api/change-server",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        if(!requireParams(req, res, {"domain"})) {
          return;
        }

        manager->changeServer(req.get_param_value("domain"));
        returnSuccess(req, res);
      });

  svr.Post(
      "/api/host/add",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        if(!requireParams(req, res, {"hostname", "address"})) {
          return;
        }

        manager->hostTableAdd(
            req.get_param_value("hostname"),
            IpAddress::parse(req.get_param_value("address")));
        returnSuccess(req, res);
      });

  svr.Post(
      "/api/host/rm", [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        if(!requireParams(req, res, {"hostname"})) {
          return;
        }

        manager->hostTableRm(req.get_param_value("hostname"));
        returnSuccess(req, res);
      });

  svr.Get(
      "/api/whitelist/ls",
      [&](const httplib::Request& req, httplib::Response& res) {
        std::vector<std::string> list;
        for(auto addr : manager->getWhitelist()) {
          list.push_back(addr.str());
        }
        returnSuccess(req, res, list);
      });

  svr.Post(
      "/api/whitelist/add",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        if(!requireParams(req, res, {"address"})) {
          return;
        }

        manager->whitelistAdd(IpAddress::parse(req.get_param_value("address")));
        returnSuccess(req, res);
      });

  svr.Post(
      "/api/whitelist/rm",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        if(!requireParams(req, res, {"address"})) {
          return;
        }

        manager->whitelistRm(IpAddress::parse(req.get_param_value("address")));
        returnSuccess(req, res);
      });

  svr.Post(
      "/api/whitelist/enable",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        manager->whitelistEnable();
        returnSuccess(req, res);
      });

  svr.Post(
      "/api/whitelist/disable",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        manager->whitelistDisable();
        returnSuccess(req, res);
      });

  svr.Post(
      "/api/hooks/enable",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        manager->hooksEnable();
        returnSuccess(req, res);
      });

  svr.Post(
      "/api/hooks/disable",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        manager->hooksDisable();
        returnSuccess(req, res);
      });

  svr.Get(
      "/api/logs/get",
      [&](const httplib::Request& req, httplib::Response& res) {
        returnSuccess(req, res, getGlobalLogManager()->getLogs());
      });

  svr.Post(
      "/api/logs/settings",
      [&](const httplib::Request& req, httplib::Response& res) {
        auto logManager = getGlobalLogManager();

        if(req.has_param("verbosity") || req.has_param("size")) {
          if(!validateSecret(req, res)) {
            return;
          }

          if(req.has_param("verbosity")) {
            if(std::stoi(req.get_param_value("verbosity")) <=
               logLevelToInt(LogLevel::CRITICAL)) {
              manager->setLogVerbosity(
                  std::stoi(req.get_param_value("verbosity")));
            }
          }
          if(req.has_param("size")) {
            if(std::stoi(req.get_param_value("size")) <= 1000 &&
               std::stoi(req.get_param_value("size")) >= 10) {
              logManager->setSize(std::stoi(req.get_param_value("size")));
            }
          }
        }

        returnSuccess(req, res);
      });

  svr.Get(
      "/api/logs/settings",
      [&](const httplib::Request& req, httplib::Response& res) {
        auto logManager = getGlobalLogManager();

        returnSuccess(
            req, res,
            {{"verbosity", logLevelToInt(logManager->getVerbosity())},
             {"size", logManager->getSize()},
             {"current_size", logManager->getCurrentSize()}});
      });

  if(!svr.bind_to_port("127.0.0.1", manager->getApiPort())) {
    LOG_CRITICAL(
        "Unable to bind HTTP thread to port 127.0.0.1:%d. Exiting!",
        manager->getApiPort());
    exit(1);
  } else {
    LOG_INFO(
        "HTTP thread bound to 127.0.0.1:%d. Will start handling the "
        "connections.",
        manager->getApiPort());
  }

  if(!svr.listen_after_bind()) {
    LOG_CRITICAL("HTTP thread finished unexpectedly. Exiting!");
    exit(1);
  }
}
