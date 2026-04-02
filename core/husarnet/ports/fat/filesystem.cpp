// Copyright (c) 2025 Husarnet sp. z o.o.
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

// readFile version with no logging for best-effort reading (for example for reading before logger is set up)
__attribute__((weak)) const std::string readFileSilent(const std::string& path)
{
  if(!isFile(path)) {
    return "";
  }

  std::ifstream f(path);
  if(!f.good()) {
    return "";
  }

  std::stringstream buffer;
  buffer << f.rdbuf();

  return buffer.str();
}

__attribute__((weak)) const std::string readFile(const std::string& path)
{
  if(!isFile(path)) {
    HLOG_ERROR("file does not exist // {path}", path);
    return "";
  }

  std::ifstream f(path);
  if(!f.good()) {
    HLOG_ERROR("failed to open file for reading // {path}", path);
    exit(1);
  }

  std::stringstream buffer;
  buffer << f.rdbuf();

  return buffer.str();
}

__attribute__((weak)) bool writeFile(const std::string& path, const std::string& data)
{
  std::ofstream f(path);
  if(!f.good()) {
    HLOG_ERROR("failed to open file for writing // {path}", path);
    return false;
  }

  f << data;

  HLOG_DEBUG("written to file directly // {path}", path);
  return true;
}

__attribute__((weak)) bool transformFile(
    const std::string& path,
    std::function<std::string(const std::string&)> transform)
{
  HLOG_DEBUG("naively transforming file // {path}", path);

  if(!isFile(path)) {
    HLOG_ERROR("file does not exist // {path}", path);
    return false;
  }

  auto currentContent = readFile(path);
  std::string newContent = transform(currentContent);
  return writeFile(path, newContent);
}
