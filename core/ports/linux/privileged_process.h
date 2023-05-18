// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ports/linux/privileged_process.h"

#include "enum.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

BETTER_ENUM(
    PrivilegedMethod,
    int,
    updateHostsFile = 1,
    getSelfHostname = 2,
    setSelfHostname = 3,
    notifyReady = 4,
    runHook = 5,
    checkHook = 6,
    resolveToIp = 7)

class PrivilegedProcess {
 private:
  int fd = -1;

  json handleUpdateHostsFile(json data);
  json handleGetSelfHostname(json data);
  json handleSetSelfHostname(json data);
  json handleNotifyReady(json data);
  json handleRunHook(json data);
  json handleCheckHookExists(json data);
  json handleResolveToIp(json data);

 public:
  PrivilegedProcess();
  void init(int fd);
  void run();
};
