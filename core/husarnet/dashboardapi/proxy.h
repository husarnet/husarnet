// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/config_manager.h"
#include "husarnet/identity.h"

#include "httplib.h"
#include "nlohmann/json.hpp"

namespace dashboardapi {

  class Proxy {
   private:
    Identity* myIdentity;
    ConfigManager* configManager;

   public:
    Proxy(Identity* myIdentity)
        : myIdentity(myIdentity)
    {
    }

    void signAndForward(const httplib::Request& req, httplib::Response& res, const std::string& path);
  };

}
