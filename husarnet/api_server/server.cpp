// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/api_server/server.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/util.h"
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
  returnSuccess(req, res, "ok");
}

void ApiServer::returnInvalidQuery(
    const httplib::Request& req,
    httplib::Response& res,
    std::string errorString)
{
  LOG("httpApi: returning invalid query: %s", errorString.c_str());
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
  LOG("httpApi: returning error: %s", errorString.c_str());
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

void ApiServer::runThread()
{
  httplib::Server svr;

  // Test endpoint
  svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("Hello World!", "text/plain");
  });

  svr.Get(
      "/control/status",
      [&](const httplib::Request& req, httplib::Response& res) {
        auto hostTable = manager->getConfigStorage().getHostTable();
        std::map<std::string, std::string> hostTableStringified;

        for(auto it = hostTable.begin(); it != hostTable.end(); it++) {
          hostTableStringified.insert({it->first, it->second.toString()});
        }

        // TODO add peer data here (ip, hostname, latency + is this a websetup
        // host data)
        returnSuccess(
            req, res,
            json::object({
                {"version", manager->getVersion()},
                {"local_ip", manager->getSelfAddress().toString()},
                {"local_hostname", manager->getSelfHostname()},
                {"is_joined", manager->isJoined()},
                {"websetup_address", manager->getWebsetupAddress().toString()},
                {"base_connection",
                 {{"type", manager->getCurrentBaseProtocol()},
                  {"address", manager->getCurrentBaseAddress().ip.toString()},
                  {"port", manager->getCurrentBaseAddress().port}}},
                {"host_table", hostTableStringified},
            }));
      });

  svr.Post(
      "/control/join",
      [&](const httplib::Request& req, httplib::Response& res) {
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
      "/control/host/add",
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
      "/control/host/rm",
      [&](const httplib::Request& req, httplib::Response& res) {
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
      "/control/whitelist/ls",
      [&](const httplib::Request& req, httplib::Response& res) {
        std::vector<std::string> list;
        for(auto addr : manager->getWhitelist()) {
          list.push_back(addr.str());
        }
        returnSuccess(req, res, list);
      });

  svr.Post(
      "/control/whitelist/add",
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
      "/control/whitelist/rm",
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
      "/control/whitelist/enable",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        manager->whitelistEnable();
        returnSuccess(req, res);
      });

  svr.Post(
      "/control/whitelist/disable",
      [&](const httplib::Request& req, httplib::Response& res) {
        if(!validateSecret(req, res)) {
          return;
        }

        manager->whitelistDisable();
        returnSuccess(req, res);
      });

  svr.Get(
      "/control/logs/get",
      [&](const httplib::Request& req, httplib::Response& res) {
        returnSuccess(req, res, manager->getLogManager().getLogs());
      });

  svr.Get(
      "/control/logs/settings",
      [&](const httplib::Request& req, httplib::Response& res) {
        auto logManager = manager->getLogManager();

        if(req.has_param("verbosity") || req.has_param("size")) {
          if(!validateSecret(req, res)) {
            return;
          }

          if(req.has_param("verbosity")) {
            logManager.setVerbosity(
                std::stoi(req.get_param_value("verbosity")));
          }
          if(req.has_param("size")) {
            logManager.setSize(std::stoi(req.get_param_value("size")));
          }
        }

        returnSuccess(
            req, res,
            {{"verbosity", logManager.getVerbosity()},
             {"size", logManager.getSize()},
             {"current_size", logManager.getCurrentSize()}});
      });

  if(!svr.bind_to_port("127.0.0.1", manager->getApiPort())) {
    LOG("Unable to bind HTTP thread to port 127.0.0.1:%d. Exiting!",
        manager->getApiPort());
    exit(1);
  } else {
    LOG("HTTP thread bound to 127.0.0.1:%d. Will start handling the "
        "connections.",
        manager->getApiPort());
  }

  if(!svr.listen_after_bind()) {
    LOG("HTTP thread finished unexpectedly. Exiting!");
    exit(1);
  }
}
