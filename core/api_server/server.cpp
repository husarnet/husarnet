#include "api_server/server.h"
#include "api_server/common.h"
#include "configmanager.h"
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "util.h"

using namespace nlohmann;  // json

namespace APIServer {

static void returnSuccess(const httplib::Request& req,
                          httplib::Response& res,
                          json result) {
  json doc;

  doc["status"] = "success";
  doc["result"] = result;

  res.set_content(doc.dump(4), "text/json");
}

static void returnSuccess(const httplib::Request& req, httplib::Response& res) {
  returnSuccess(req, res, "ok");
}

static void returnInvalidQuery(const httplib::Request& req,
                               httplib::Response& res,
                               std::string errorString) {
  LOG("httpApi: returning invalid query: %s", errorString.c_str());
  json doc;

  doc["status"] = "invalid";
  doc["error"] = errorString;

  res.set_content(doc.dump(4), "text/json");
}

static void returnError(const httplib::Request& req,
                        httplib::Response& res,
                        std::string errorString) {
  LOG("httpApi: returning error: %s", errorString.c_str());
  json doc;

  doc["status"] = "error";
  doc["error"] = errorString;

  res.set_content(doc.dump(4), "text/json");
}

static bool validateSecret(const httplib::Request& req,
                           httplib::Response& res) {
  if (!req.has_param("secret") ||
      req.get_param_value("secret") != getApiSecret()) {
    returnInvalidQuery(req, res, "invalid control secret");
    return false;
  }

  return true;
}

static bool requireParams(const httplib::Request& req,
                          httplib::Response& res,
                          std::list<std::string> paramNames) {
  for (auto paramName : paramNames) {
    if (!req.has_param(paramName.c_str())) {
      returnInvalidQuery(req, res, "missing param " + paramName);
      return false;
    }
  }
  return true;
}

void httpThread(ConfigManager& configManager) {
  httplib::Server svr;

  // Test endpoint
  svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("Hello World!", "text/plain");
  });

  svr.Get("/control/status", [&](const httplib::Request& req,
                                 httplib::Response& res) {
    returnSuccess(req, res,
                  {{"version", configManager.getVersion()},
                   {"local_ip", configManager.getSelfAddress()},
                   {"local_hostname", configManager.getSelfHostname()},
                   {"is_joined", configManager.isJoined()},
                   {"base_connection",
                    {{"type", configManager.getCurrentBaseProtocol()},
                     {"address", configManager.getCurrentBaseAddress()}}}});
  });

  svr.Post("/control/join",
           [&](const httplib::Request& req, httplib::Response& res) {
             if (!validateSecret(req, res)) {
               return;
             }

             if (!requireParams(req, res, {"code", "hostname"})) {
               return;
             }

             std::string code = req.get_param_value("code");
             std::string hostname = req.get_param_value("hostname");
             configManager.joinNetwork(code, hostname);
             returnSuccess(req, res);
           });

  svr.Post("/control/host/add", [&](const httplib::Request& req,
                                    httplib::Response& res) {
    if (!validateSecret(req, res)) {
      return;
    }

    if (!requireParams(req, res, {"hostname", "address"})) {
      return;
    }

    try {
      configManager.hostTableAdd(
          req.get_param_value("hostname"),
          IpAddress::parse(req.get_param_value("address")));
      returnSuccess(req, res);
    } catch (ConfigEditFailed& err) {
      returnError(req, res, std::string("could not add host ") + err.what());
    }
  });

  svr.Post("/control/host/rm", [&](const httplib::Request& req,
                                   httplib::Response& res) {
    if (!validateSecret(req, res)) {
      return;
    }

    if (!requireParams(req, res, {"hostname"})) {
      return;
    }

    try {
      configManager.hostTableRm(req.get_param_value("hostname"));
      returnSuccess(req, res);
    } catch (ConfigEditFailed& err) {
      returnError(req, res, std::string("could not rm host ") + err.what());
    }
  });

  svr.Get("/control/whitelist/ls",
          [&](const httplib::Request& req, httplib::Response& res) {
            std::vector<std::string> list;
            for (auto addr : configManager.getWhitelist()) {
              list.push_back(addr.str());
            }
            returnSuccess(req, res, list);
          });

  svr.Post("/control/whitelist/add", [&](const httplib::Request& req,
                                         httplib::Response& res) {
    if (!validateSecret(req, res)) {
      return;
    }

    if (!requireParams(req, res, {"address"})) {
      return;
    }

    try {
      configManager.whitelistAdd(
          IpAddress::parse(req.get_param_value("address")));
      returnSuccess(req, res);
    } catch (ConfigEditFailed& err) {
      returnError(req, res,
                  std::string("could not add host to whitelist ") + err.what());
    }
  });

  svr.Post("/control/whitelist/rm", [&](const httplib::Request& req,
                                        httplib::Response& res) {
    if (!validateSecret(req, res)) {
      return;
    }

    if (!requireParams(req, res, {"address"})) {
      return;
    }

    try {
      configManager.whitelistRm(
          IpAddress::parse(req.get_param_value("address")));
      returnSuccess(req, res);
    } catch (ConfigEditFailed& err) {
      returnError(
          req, res,
          std::string("could not remove host from whitelist ") + err.what());
    }
  });

  svr.Post("/control/whitelist/enable", [&](const httplib::Request& req,
                                            httplib::Response& res) {
    if (!validateSecret(req, res)) {
      return;
    }

    try {
      configManager.whitelistEnable();
      returnSuccess(req, res);
    } catch (ConfigEditFailed& err) {
      returnError(req, res,
                  std::string("could not enable whitelist ") + err.what());
    }
  });

  svr.Post("/control/whitelist/disable", [&](const httplib::Request& req,
                                             httplib::Response& res) {
    if (!validateSecret(req, res)) {
      return;
    }

    try {
      configManager.whitelistDisable();
      returnSuccess(req, res);
    } catch (ConfigEditFailed& err) {
      returnError(req, res,
                  std::string("could not disable whitelist ") + err.what());
    }
  });

  svr.Get("/control/logs/get",
          [&](const httplib::Request& req, httplib::Response& res) {
            returnSuccess(req, res, configManager.logManager.getLogs());
          });

  svr.Get("/control/logs/settings", [&](const httplib::Request& req,
                                        httplib::Response& res) {
    // najpierw przenalizuj argumenty i zmien ustawienia a potem wypluj slownik
    // z aktualnymi
    if (req.has_param("verbosity") || req.has_param("size")) {
      if (!validateSecret(req, res)) {
        return;
      }
      if (req.has_param("verbosity")) {
        configManager.logManager.setVerbosity(req.get_param_value("verbosity"));
      }
      if (req.has_param("size")) {
        configManager.logManager.setSize(req.get_param_value("size"));
      }
    }

    returnSuccess(
        req, res,
        {{"verbosity", configManager.logManager.getVerbosity()},
         {"size", configManager.logManager.getSize()},
         {"current_size", configManager.logManager.getCurrentSize()}});
  });

  if (!svr.bind_to_port("127.0.0.1", getApiPort())) {
    LOG("Unable to bind HTTP thread to port 127.0.0.1:%d. Exiting!",
        getApiPort());
    exit(1);
  } else {
    LOG("HTTP thread bound to 127.0.0.1:%d. Will start handling the "
        "connections.",
        getApiPort());
  }

  if (!svr.listen_after_bind()) {
    LOG("HTTP thread finished unexpectedly. Exiting!");
    exit(1);
  }
}

}  // namespace APIServer
