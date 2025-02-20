// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

#include <functional>
#include <string>

#include "husarnet/logging.h"
#include "husarnet/util.h"

bool isFile(const std::string& path);

const std::string readFile(const std::string& path);

// This is the most naive implementation possible - open at the beginning of a
// file, dump the content, close the file Use this only for files that are not
// shared with other processes
bool writeFile(const std::string& path, const std::string& data);

// This is a method for handling files that are shared with other processes in
// the most graceful way possible Default implementation is still somewhat
// naive, but it can be overriden by a given port to make it smarter
bool transformFile(
    const std::string& path,
    std::function<std::string(const std::string&)> transform);
