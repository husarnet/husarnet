// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <cstdarg>
#include <cstdio>

#include <enum.h>

#include "husarnet/ports/port_interface.h"

#include "husarnet/logmanager.h"

// Windows API is broken
#undef ERROR
BETTER_ENUM(LogLevel, int, DEBUG, INFO, WARNING, ERROR, CRITICAL)

static inline const std::string pad(int minLength, const std::string& text)
{
  int padSize = 0;

  if(text.length() < minLength) {
    padSize = minLength - text.length();
  }

  return text + std::string(padSize, ' ');
}

// Do not use this function directly - instead use the macros below
static inline void log(
    LogLevel level,
    const std::string& filename,
    int lineno,
    const std::string& extra,
    const char* format,
    ...)
{
  int buffer_len = 256;
  char buffer[buffer_len];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, buffer_len, format, args);
  va_end(args);

  auto userMessage = std::string(buffer);
  if(extra.length() > 0) {
    userMessage += " " + extra;
  }

  userMessage = pad(80, userMessage);
  auto levelName = pad(8, level._to_string());

  snprintf(
      buffer, buffer_len, "%s %s (%s:%d)", levelName.c_str(),
      userMessage.c_str(), filename.c_str(), lineno);

  auto message = std::string(buffer);

  Port::log(message);
  // TODO reenable this after adding some synchronization primitvies to it
  // getGlobalLogManager()->insert(message);
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
