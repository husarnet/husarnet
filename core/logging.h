// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <cstdarg>
#include <cstdio>

#include <enum.h>

#include "husarnet/ports/port_interface.h"

#include "husarnet/logmanager.h"
#include "husarnet/util.h"

const std::string stripLogPathPrefix(const std::string& filename);

// Do not use this function directly - instead use the macros below
// TODO: replace buffer with std::string
static inline void log(
    LogLevel level,
    const std::string& filename,
    int lineno,
    const std::string& extra,
    const char* format,
    ...)
{
  if(level > getGlobalLogManager()->getVerbosity() || level == LogLevel::NONE) {
    return;
  }

  int buffer_len = 256;
  char buffer[buffer_len];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, buffer_len, format, args);
  va_end(args);

  std::string userMessage = buffer;
  if(extra.length() > 0) {
    userMessage += " " + extra;
  }

  // TODO: this is not a cleanest solution, but it works for now

  std::string message;
#ifndef ESP_PLATFORM
  message = Port::getHumanTime();
  message += " " + padRight(8, level._to_string());
#endif

#if defined(DEBUG_BUILD) && not defined(ESP_PLATFORM)
  message += " " + padRight(80, userMessage);
  message +=
      " (" + stripLogPathPrefix(filename) + ":" + std::to_string(lineno) + ")";
#else
  message += " " + userMessage;
#endif

  Port::log(level, message);

#ifndef ESP_PLATFORM
  getGlobalLogManager()->insert(message);
#endif
}

// Legacy compatibility alias
#define LOG(fmt, x...)                                        \
  {                                                           \
    log(LogLevel::WARNING, __FILE__, __LINE__, "", fmt, ##x); \
  }

// New log level aliases
#ifdef DEBUG_BUILD
#define LOG_DEBUG(fmt, x...)                                \
  {                                                         \
    log(LogLevel::DEBUG, __FILE__, __LINE__, "", fmt, ##x); \
  }
#else
// This is a hacky solution but should still emit optimized code and suppress
// the unused variable warnings for production builds
#define LOG_DEBUG(fmt, x...)                                  \
  {                                                           \
    if(false) {                                               \
      log(LogLevel::DEBUG, __FILE__, __LINE__, "", fmt, ##x); \
    }                                                         \
  }
#endif

#define LOG_INFO(fmt, x...)                                \
  {                                                        \
    log(LogLevel::INFO, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define LOG_WARNING(fmt, x...)                                \
  {                                                           \
    log(LogLevel::WARNING, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define LOG_ERROR(fmt, x...)                                \
  {                                                         \
    log(LogLevel::ERROR, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define LOG_CRITICAL(fmt, x...)                                \
  {                                                            \
    log(LogLevel::CRITICAL, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define LOG_SOCKETERR(fmt, x...)                                         \
  {                                                                      \
    log(LogLevel::ERROR, __FILE__, __LINE__, strerror(errno), fmt, ##x); \
  }

#define error_negative(ret, fmt, x...)                                   \
  {                                                                      \
    if(ret == 0)                                                         \
      return;                                                            \
                                                                         \
    log(LogLevel::ERROR, __FILE__, __LINE__, strerror(errno), fmt, ##x); \
  }
