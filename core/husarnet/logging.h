// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

// Windows API is broken
#undef ERROR

enum class LogLevel
{
  NONE = 0,
  CRITICAL = 1,
  ERROR = 2,
  WARNING = 3,
  INFO = 4,
  DEBUG = 5,
};

extern LogLevel globalLogLevel;

// Do not use this function directly
// Do not use Port::log function directly either
// Do use the LOG_* macros below
void log(
    LogLevel level,
    const std::string& filename,
    int lineno,
    const std::string& extra,
    const char* format,
    ...);

// New log level aliases

#define LOG_DEBUG(fmt, x...)                                \
  {                                                         \
    log(LogLevel::DEBUG, __FILE__, __LINE__, "", fmt, ##x); \
  }

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

#define LOG_NEGATIVE(ret, fmt, x...)                                       \
  {                                                                        \
    if(ret < 0) {                                                          \
      log(LogLevel::ERROR, __FILE__, __LINE__, strerror(errno), fmt, ##x); \
    }                                                                      \
  }

#define error_negative(ret, fmt, x...)                                   \
  {                                                                      \
    if(ret == 0)                                                         \
      return;                                                            \
                                                                         \
    log(LogLevel::ERROR, __FILE__, __LINE__, strerror(errno), fmt, ##x); \
  }
