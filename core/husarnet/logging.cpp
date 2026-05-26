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

#if not defined(ESP_PLATFORM)

// Windows API is broken
#undef ERROR

quill::Logger* logger;
quill::LogLevel husarnetLogLevelToQuill(LogLevel level)
{
  if(level == LogLevel::NONE) {
    return quill::LogLevel::None;
  }
  if(level == LogLevel::CRITICAL) {
    return quill::LogLevel::Critical;
  }
  if(level == LogLevel::ERROR) {
    return quill::LogLevel::Error;
  }
  if(level == LogLevel::WARNING) {
    return quill::LogLevel::Warning;
  }
  if(level == LogLevel::INFO) {
    return quill::LogLevel::Info;
  }

  return quill::LogLevel::Debug;
}

void initLogging(LogLevel husarnetLogLevel, bool jsonLogging)
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  if(jsonLogging) {
    // PatternFormatter is only used for non-structured logs formatting
    // When logging only json, it is ideal to set the logging pattern to empty to avoid unnecessary message formatting.
    auto jsonSink = quill::Frontend::create_or_get_sink<HusarnetJsonSink>("sink_1");
    logger = quill::Frontend::create_or_get_logger("main_logger", std::move(jsonSink));
    logger->set_log_level(husarnetLogLevelToQuill(husarnetLogLevel));
    return;
  }

  auto noJsonSink = quill::Frontend::create_or_get_sink<HusarnetNoJsonSink>("sink_1");
  logger = quill::Frontend::create_or_get_logger(
      "main_logger", std::move(noJsonSink),
      quill::PatternFormatterOptions{"%(time) %(log_level:<7) %(short_source_location:<28) ", "%H:%M:%S.%Qns"});
  logger->set_log_level(husarnetLogLevelToQuill(husarnetLogLevel));
  return;
}

#else  // non-FAT platforms

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

  message += userMessage;
  Port::log(level, message);
}

void initLogging(LogLevel husarnetLogLevel, bool jsonLogging)
{
  globalLogLevel = husarnetLogLevel;
  (void)jsonLogging;  // json logging is not supported on non-FAT platforms
}

#endif  // PORT_FAT
