// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/util.h"

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

#define HUSARNET_LOG_DEBUG(fmt, x...)                       \
  {                                                         \
    log(LogLevel::DEBUG, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define HUSARNET_LOG_INFO(fmt, x...)                       \
  {                                                        \
    log(LogLevel::INFO, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define HUSARNET_LOG_WARNING(fmt, x...)                       \
  {                                                           \
    log(LogLevel::WARNING, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define HUSARNET_LOG_ERROR(fmt, x...)                       \
  {                                                         \
    log(LogLevel::ERROR, __FILE__, __LINE__, "", fmt, ##x); \
  }

#define HUSARNET_LOG_CRITICAL(fmt, x...)                       \
  {                                                            \
    log(LogLevel::CRITICAL, __FILE__, __LINE__, "", fmt, ##x); \
  }

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/backend/PatternFormatter.h"
#include "quill/core/Attributes.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/JsonSink.h"
#include "quill/sinks/StreamSink.h"

class HusarnetSink : public quill::StreamSink {
 public:
  using quill::StreamSink::StreamSink;

  HusarnetSink() : quill::StreamSink("stdout", nullptr)
  {
  }

  ~HusarnetSink() override = default;

 protected:
  size_t getMessageUpperBound(char const* message_format)
  {
    size_t i = 0;
    size_t upperBound = 0;
    while(message_format[i] != '\0') {
      i++;
      if(message_format[i - 1] == '/' && message_format[i] == '/') {
        upperBound = i - 2;
        break;
      }
      upperBound = i;
    }
    return upperBound;
  }
  fmtquill::memory_buffer _message;
  std::string _format;
};

class HusarnetJsonSink : public HusarnetSink {
  QUILL_ATTRIBUTE_HOT void write_log(
      quill::MacroMetadata const* log_metadata,
      uint64_t log_timestamp,
      std::string_view thread_id,
      std::string_view thread_name,
      std::string const& process_id,
      std::string_view logger_name,
      quill::LogLevel log_level,
      std::string_view log_level_description,
      std::string_view log_level_short_code,
      std::vector<std::pair<std::string, std::string>> const* named_args,
      std::string_view log_message,
      std::string_view log_statement) override
  {
    auto message_format = log_metadata->message_format();
    auto upperBound = getMessageUpperBound(message_format);
    std::string_view msg(message_format, upperBound);

    _message.append(fmtquill::format(
        R"({{"timestamp":"{}","log_level":"{}","message":"{}")", std::to_string(log_timestamp), log_level_description,
        msg));

    // add log statement arguments as key-values to the json
    if(named_args) {
      for(auto const& [key, value] : *named_args) {
        _message.append(std::string_view{",\""});
        _message.append(key);
        _message.append(std::string_view{"\":\""});
        _message.append(value);
        _message.append(std::string_view{"\""});
      }
    }

    _message.append(std::string_view{"}\n"});

    StreamSink::write_log(
        log_metadata, log_timestamp, thread_id, thread_name, process_id, logger_name, log_level, log_level_description,
        log_level_short_code, named_args, std::string_view{}, std::string_view{_message.data(), _message.size()});

    _message.clear();
  }
};

class HusarnetNoJsonSink : public HusarnetSink {
  QUILL_ATTRIBUTE_HOT void write_log(
      quill::MacroMetadata const* log_metadata,
      uint64_t log_timestamp,
      std::string_view thread_id,
      std::string_view thread_name,
      std::string const& process_id,
      std::string_view logger_name,
      quill::LogLevel log_level,
      std::string_view log_level_description,
      std::string_view log_level_short_code,
      std::vector<std::pair<std::string, std::string>> const* named_args,
      std::string_view log_message,
      std::string_view log_statement) override
  {
    auto message_format = log_metadata->message_format();
    auto upperBound = getMessageUpperBound(message_format);
    std::string msg(message_format, upperBound);

    std::string stmt(log_statement.substr(0, log_statement.size() - 1));  // TODO check WIN32
    stmt.append(trim(msg));

    int i = 0;
    int numArgs = named_args ? named_args->size() : 0;

    if(named_args) {
      stmt.append(" (");
      for(auto const& [key, value] : *named_args) {
        i++;
        stmt.append(key);
        stmt.append("=");
        stmt.append(value);
        if(i != numArgs) {
          stmt.append(", ");
        }
      }
      stmt.append(")");
    }

    stmt.append("\n");

    StreamSink::write_log(
        log_metadata, log_timestamp, thread_id, thread_name, process_id, logger_name, log_level, log_level_description,
        log_level_short_code, named_args, msg, std::string_view{stmt.data(), stmt.size()});
  }
};

quill::Logger* initLogging(bool jsonLogging);
quill::LogLevel husarnetLogLevelToQuill(LogLevel level);

extern quill::Logger* logger;
