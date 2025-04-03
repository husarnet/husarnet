// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/ports/port_interface.h"

#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"

#include "etl/map.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

class ConfigEnv {
 private:
  etl::map<EnvKey, std::string, ENV_KEY_OPTIONS> env;

 public:
  ConfigEnv();

  json getJson() const;  // To be put i.e. in status/doctor

  std::string getTldFqdn() const;
  LogLevel getLogVerbosity() const;
  bool getEnableHooks() const;
  bool getEnableControlplane() const;
  const std::string getDaemonInterface() const;
  const std::string getDaemonApiInterface() const;
  const InternetAddress getDaemonApiHost() const;
  int getDaemonApiPort() const;
};
