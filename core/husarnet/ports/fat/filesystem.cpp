// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

#include "husarnet/ports/fat/filesystem.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "husarnet/logging.h"
#include "husarnet/util.h"

__attribute__((weak)) bool isFile(const std::string& path)
{
  return std::filesystem::exists(path);
}

__attribute__((weak)) std::string readFile(const std::string& path)
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

__attribute__((weak)) bool writeFile(
    const std::string& path,
    const std::string& data)
{
  std::ofstream f(path);
  if(!f.good()) {
    LOG_ERROR("failed to open %s for writing", path.c_str());
    return false;
  }

  f << data;
  return true;
}

__attribute__((weak)) bool transformFile(
    const std::string& path,
    std::function<std::string(const std::string&)> transform)
{
  if(!isFile(path)) {
    LOG_ERROR("file %s does not exist", path.c_str());
    return false;
  }

  std::string currentContent = readFile(path);
  std::string newContent = transform(currentContent);
  return writeFile(path, newContent);
}
