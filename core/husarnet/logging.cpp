// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/logging.h"

#include <cstdarg>
#include <cstdio>
#include <string>

#include "husarnet/ports/port_interface.h"

#include "husarnet/util.h"

#include "magic_enum/magic_enum.hpp"

// This is merely a starting/bootstrap log level. As soon as the config is
// loaded, proper one will be set
#ifdef DEBUG_BUILD
LogLevel globalLogLevel = LogLevel::DEBUG;
#else
LogLevel globalLogLevel = LogLevel::INFO;
#endif

// This needs to live in a .cpp file due to __FILE__ macro usage and the way we
// handle tempIncludes in CMake
// cppcheck-suppress unusedFunction
const static std::string stripLogPathPrefix(const std::string& filename)
{
  auto loggingFile = std::string(__FILE__);
  auto prefix = loggingFile.substr(0, loggingFile.length() - std::string("/core/husarnet/logging.cpp").length() + 1);
  return filename.substr(prefix.length());
}

// TODO: replace buffer with std::string
void log(LogLevel level, const std::string& filename, int lineno, const std::string& extra, const char* format, ...)
{
  if(globalLogLevel < level) {
    return;
  }

  int buffer_len = 256;
  char buffer[buffer_len];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, buffer_len, format, args);
  va_end(args);

  std::string userMessage{buffer};
  if(extra.length() > 0) {
    userMessage += " " + extra;
  }

  // TODO: this is not a cleanest solution, but it works for now

  std::string message;

  // ESP-IDF exposes human readable time internally so there's no point in
  // duplicating it
#ifndef ESP_PLATFORM
  message = Port::getHumanTime();
  message += " " + padRight(8, std::string(magic_enum::enum_name(level)));
#endif

#if defined(DEBUG_BUILD) && not defined(ESP_PLATFORM)
  message += " " + padRight(80, userMessage);
  message += " (" + stripLogPathPrefix(filename) + ":" + std::to_string(lineno) + ")";
#else
  message += userMessage;
#endif

  Port::log(level, message);
}
