// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/identity.h"

#include "httplib.h"
#include "nlohmann/json.hpp"

class DashboardApiProxy {
 private:
  Identity* identity;
  const std::string apiAddress;
  httplib::Client httpClient;

 public:
  DashboardApiProxy(Identity* identityPtr, const std::string& addr)
      : identity(identityPtr), apiAddress(addr), httpClient(addr)
  {
  }
  bool isValid();
  void signAndForward(
      const httplib::Request& req,
      httplib::Response& res,
      const std::string& path);
};
