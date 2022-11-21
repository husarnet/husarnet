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
    const char* format,
    ...)
{
  int buffer_len = 256;
  char buffer[buffer_len];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, buffer_len, format, args);
  va_end(args);

  auto userMessage = pad(80, buffer);
  auto levelName = pad(8, level._to_string());

  snprintf(
      buffer, buffer_len, "%s %s (%s:%d)", levelName.c_str(),
      userMessage.c_str(), filename.c_str(), lineno);

  auto message = std::string(buffer);

  Port::log(message);
  getGlobalLogManager()->insert(message);
}

// Legacy compatibility aliases
#define LOG(fmt, x...)                                    \
  {                                                       \
    log(LogLevel::WARNING, __FILE__, __LINE__, fmt, ##x); \
  }

#define LOGV(fmt, x...)                                \
  {                                                    \
    log(LogLevel::INFO, __FILE__, __LINE__, fmt, ##x); \
  }

// New log level aliases
#ifdef DEBUG_BUILD
#define LOG_DEBUG(fmt, x...)                            \
  {                                                     \
    log(LogLevel::DEBUG, __FILE__, __LINE__, fmt, ##x); \
  }
#else
#define LOG_DEBUG(fmt, x...) \
  {                          \
  }
#endif

#define LOG_INFO(fmt, x...)                            \
  {                                                    \
    log(LogLevel::INFO, __FILE__, __LINE__, fmt, ##x); \
  }

#define LOG_WARNING(fmt, x...)                            \
  {                                                       \
    log(LogLevel::WARNING, __FILE__, __LINE__, fmt, ##x); \
  }

#define LOG_ERROR(fmt, x...)                            \
  {                                                     \
    log(LogLevel::ERROR, __FILE__, __LINE__, fmt, ##x); \
  }

#define LOG_CRITICAL(fmt, x...)                            \
  {                                                        \
    log(LogLevel::CRITICAL, __FILE__, __LINE__, fmt, ##x); \
  }
