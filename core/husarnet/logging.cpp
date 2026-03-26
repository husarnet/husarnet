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

/**
 * Overrides generate_json_message to use a custom json format
 */
void HusarnetLoggingSink::generate_json_message(quill::MacroMetadata const* /** log_metadata **/, uint64_t log_timestamp,
                           std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                           std::string const& /** process_id **/, std::string_view /** logger_name **/,
                           quill::LogLevel /** log_level **/, std::string_view log_level_description,
                           std::string_view /** log_level_short_code **/,
                           std::vector<std::pair<std::string, std::string>> const* named_args,
                           std::string_view /** log_message **/,
                           std::string_view /** log_statement **/, char const* message_format)
{
  size_t i = 0;
  size_t upperBound = 0;
  while (message_format[i] != '\0')
  {
    i++;
    if (message_format[i-1] == '/' && message_format[i] == '/')
    {
      upperBound = i-2;
      break;
    }
    upperBound = i;
  }


  std::string_view msg(message_format, upperBound);

  // format json as desired
  _json_message.append(fmtquill::format(R"({{"timestamp":"{}","log_level":"{}","message":"{}")",
                                        std::to_string(log_timestamp), log_level_description, msg));

  // add log statement arguments as key-values to the json
  if (named_args)
  {
    for (auto const& [key, value] : *named_args)
    {
      _json_message.append(std::string_view{",\""});
      _json_message.append(key);
      _json_message.append(std::string_view{"\":\""});
      _json_message.append(value);
      _json_message.append(std::string_view{"\""});
    }
  }
}

quill::Logger* logger = initLogging(); // global logger instance

quill::Logger* initLogging() {
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Create a json sink
  auto json_sink = quill::Frontend::create_or_get_sink<HusarnetLoggingSink>("json_sink_1");

  // PatternFormatter is only used for non-structured logs formatting
  // When logging only json, it is ideal to set the logging pattern to empty to avoid unnecessary message formatting.
  return quill::Frontend::create_or_get_logger(
    "json_logger", std::move(json_sink),
    quill::PatternFormatterOptions{"", "%H:%M:%S.%Qns", quill::Timezone::GmtTime});
}


quill::LogLevel husarnetLogLevelToQuill(LogLevel level) {
  if (level == LogLevel::NONE) {
    return quill::LogLevel::None;
  }
  if (level == LogLevel::CRITICAL) {
    return quill::LogLevel::Critical;
  }
  if (level == LogLevel::ERROR) {
    return quill::LogLevel::Error;
  }
  if (level == LogLevel::WARNING) {
    return quill::LogLevel::Warning;
  }
  if (level == LogLevel::INFO) {
    return quill::LogLevel::Info;
  }

  return quill::LogLevel::Debug;
}
