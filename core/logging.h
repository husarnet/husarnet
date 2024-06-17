// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <cstdarg>
#include <cstdio>

#include <enum.h>

#include "husarnet/util.h"

const std::string stripLogPathPrefix(const std::string& filename);

// Windows API is broken
#undef ERROR

BETTER_ENUM(
    LogLevel,
    int,
    NONE, /* 0 */
    CRITICAL /* 1 */,
    ERROR /* 2 */,
    WARNING /* 3 */,
    INFO /* 4 */,
    DEBUG /* 5 */);

// Do not use this function directly - instead use the macros below
// Do not use Port::log function directly either
void log(
    LogLevel level,
    const std::string& filename,
    int lineno,
    const std::string& extra,
    const char* format,
    ...);

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
