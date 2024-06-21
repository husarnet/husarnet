// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include <husarnet/husarnet_manager.h>

#include "husarnet/identity.h"

#include "httplib.h"
#include "nlohmann/json.hpp"

class DashboardApiProxy {
 private:
  HusarnetManager* manager;

 public:
  DashboardApiProxy(HusarnetManager* manager) : manager(manager) {}

  void signAndForward(
      const httplib::Request& req,
      httplib::Response& res,
      const std::string& path);
};
