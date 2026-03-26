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
void log(LogLevel level, const std::string& filename, int lineno, const std::string& extra, const char* format, ...);

// New log level aliases

#define HUSARNET_LOG_DEBUG(fmt, x...)                                \
  {                                                         \
    log(LogLevel::DEBUG, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define HUSARNET_LOG_INFO(fmt, x...)                                \
  {                                                        \
    log(LogLevel::INFO, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define HUSARNET_LOG_WARNING(fmt, x...)                                \
  {                                                           \
    log(LogLevel::WARNING, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define HUSARNET_LOG_ERROR(fmt, x...)                                \
  {                                                         \
    log(LogLevel::ERROR, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define HUSARNET_LOG_CRITICAL(fmt, x...)                                \
  {                                                            \
    log(LogLevel::CRITICAL, __FILE__, __LINE__, "", fmt, ##x); \
  }

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/JsonSink.h"

class HusarnetLoggingSink : public quill::JsonConsoleSink
{
  void generate_json_message(quill::MacroMetadata const* /** log_metadata **/, uint64_t log_timestamp,
                             std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                             std::string const& /** process_id **/, std::string_view /** logger_name **/,
                             quill::LogLevel /** log_level **/, std::string_view log_level_description,
                             std::string_view /** log_level_short_code **/,
                             std::vector<std::pair<std::string, std::string>> const* named_args,
                             std::string_view /** log_message **/,
                             std::string_view /** log_statement **/, char const* message_format) override;
};

quill::Logger* initLogging();
quill::LogLevel husarnetLogLevelToQuill(LogLevel level);

extern quill::Logger* logger;
