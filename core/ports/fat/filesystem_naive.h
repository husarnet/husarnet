// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "husarnet/logging.h"
#include "husarnet/util.h"

// Those are the most basic, naive implementations of filesystem operations.
// Knowing that - use them only for clear situations like - daemon is the only
// thing that changes those files, etc.

static inline std::string readFile(const std::string& path)
{
  std::ifstream f(path);
  if(!f.good()) {
    LOG_ERROR("failed to open %s for reading", path.c_str());
    exit(1);
  }

  std::stringstream buffer;
  buffer << f.rdbuf();

  return buffer.str();
}

static inline bool writeFile(const std::string& path, const std::string& data)
{
  std::ofstream f(path);
  if(!f.good()) {
    LOG_ERROR("failed to open %s for writing", path.c_str());
    return false;
  }

  f << data;
  return true;
}

static inline bool isFile(const std::string& path)
{
  return std::filesystem::exists(path);
}
