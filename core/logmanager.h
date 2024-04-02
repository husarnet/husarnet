// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <chrono>
#include <iomanip>
#include <list>
#include <mutex>
#include <sstream>
#include <string>

#include <enum.h>

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

static inline LogLevel logLevelFromInt(int value)
{
  if(value > LogLevel::DEBUG)
    return LogLevel::DEBUG;

  auto level = LogLevel::_from_integral_nothrow(value);

  if(level)
    return level.value();
  else {
    return LogLevel::CRITICAL;
  }
}

static inline int logLevelToInt(LogLevel value)
{
  return value._to_integral();
}

class LogManager {
  class LogElement {
   public:
    std::string log;
    LogElement* next;
    LogElement* prev;
    explicit LogElement(std::string log)
        : log(log), next(nullptr), prev(nullptr){};
  };
  uint16_t size;
  uint16_t currentSize;
  LogElement* first;
  LogElement* last;
  LogLevel verbosity;
  std::mutex mtx;

 public:
  explicit LogManager(uint16_t size)
      : size(size),
        currentSize(0),
        first(nullptr),
        last(nullptr),
        verbosity(LogLevel::INFO){};
  std::list<std::string> getLogs();
  void setSize(uint16_t size);
  void insert(std::string& log);
  void setVerbosity(LogLevel verb);
  LogLevel getVerbosity();
  uint16_t getSize();
  uint16_t getCurrentSize();
};

extern LogManager* globalLogManager;
inline LogManager* getGlobalLogManager()
{
  if(globalLogManager == nullptr) {
    globalLogManager = new LogManager(100);
  }

  return globalLogManager;
}
