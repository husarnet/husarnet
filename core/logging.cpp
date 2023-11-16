// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/logging.h"

// This needs to live in a .cpp file due to __FILE__ macro usage and the way we
// handle tempIncludes in CMake
// cppcheck-suppress unusedFunction
const std::string stripLogPathPrefix(const std::string& filename)
{
  auto loggingFile = std::string(__FILE__);
  auto prefix = loggingFile.substr(
      0, loggingFile.length() - std::string("/core/logging.cpp").length() + 1);
  return filename.substr(prefix.length());
}
