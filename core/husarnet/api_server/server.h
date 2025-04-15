// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

#include "husarnet/config_env.h"
#include "husarnet/config_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ngsocket.h"

#include "httplib.h"
#include "nlohmann/json.hpp"

namespace httplib {
  struct Request;
  struct Response;
}  // namespace httplib

class HusarnetManager;

class ApiServer {
 private:
  ConfigEnv* configEnv;
  ConfigManager* configManager;
  HusarnetManager* husarnetManager;
  Identity* identity;

  std::mutex mutex;
  std::condition_variable cv;
  bool isReady = false;
  std::string daemonApiToken;

  void returnSuccess(const httplib::Request& req, httplib::Response& res, nlohmann::json result);
  void returnSuccess(const httplib::Request& req, httplib::Response& res);

  void returnInvalidQuery(const httplib::Request& req, httplib::Response& res, std::string errorString);
  void returnError(const httplib::Request& req, httplib::Response& res, std::string errorString);

  bool validateSecret(const httplib::Request& req, httplib::Response& res);
  void forwardRequestToDashboardApi(const httplib::Request& req, httplib::Response& res);

  bool requireParams(const httplib::Request& req, httplib::Response& res, std::list<std::string> paramNames);

 public:
  ApiServer(ConfigEnv* configEnv, ConfigManager* configManager, HusarnetManager* husarnetManager, Identity* identity);

  void runThread();
  void waitStarted();
};
