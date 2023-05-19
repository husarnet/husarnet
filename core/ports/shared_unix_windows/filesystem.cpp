// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/shared_unix_windows/filesystem.h"

#include <fstream>
#include <sstream>
#include <string>

#include "husarnet/logging.h"

namespace Port {
  std::string readFile(const std::string& path)
  {
    std::ifstream f(path);
    if(!f.good()) {
      LOG_ERROR("failed to open %s", path.c_str());
      exit(1);
    }

    std::stringstream buffer;
    buffer << f.rdbuf();

    return buffer.str();
  }
}  // namespace Port
