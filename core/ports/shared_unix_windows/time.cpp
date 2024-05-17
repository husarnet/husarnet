// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include "husarnet/ports/port_interface.h"

#include "husarnet/util.h"

namespace Port {
  // cppcheck-suppress unusedFunction
  const std::string getHumanTime()
  {
    auto now = std::chrono::system_clock::now();

    std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
    std::tm localtime = *std::localtime(&timestamp);

    return std::to_string(localtime.tm_year + 1900) + "-" +
           padLeft(2, std::to_string(localtime.tm_mon + 1), '0') + "-" +
           padLeft(2, std::to_string(localtime.tm_mday), '0') + "_" +
           padLeft(2, std::to_string(localtime.tm_hour), '0') + ":" +
           padLeft(2, std::to_string(localtime.tm_min), '0') + ":" +
           padLeft(2, std::to_string(localtime.tm_sec), '0');
  }
}  // namespace Port
