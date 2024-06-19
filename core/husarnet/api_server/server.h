// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

#include "husarnet/api_server/dashboard_api_proxy.h"

#include "httplib.h"
#include "nlohmann/json.hpp"

class HusarnetManager;

namespace httplib {
  struct Request;
  struct Response;
}  // namespace httplib

class ApiServer {
 private:
  HusarnetManager* manager;
  DashboardApiProxy* proxy;
  std::mutex mutex;
  std::condition_variable cv;

  void returnSuccess(
      const httplib::Request& req,
      httplib::Response& res,
      nlohmann::json result);
  void returnSuccess(const httplib::Request& req, httplib::Response& res);

  void returnInvalidQuery(
      const httplib::Request& req,
      httplib::Response& res,
      std::string errorString);
  void returnError(
      const httplib::Request& req,
      httplib::Response& res,
      std::string errorString);

  bool validateSecret(const httplib::Request& req, httplib::Response& res);

  nlohmann::json getStandardReply();

  bool requireParams(
      const httplib::Request& req,
      httplib::Response& res,
      std::list<std::string> paramNames);

 public:
  ApiServer(HusarnetManager* manager, DashboardApiProxy* proxy);

  void runThread();
  void waitStarted();
};
