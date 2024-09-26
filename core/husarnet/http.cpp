// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/http.h"

#include <etl/string_view.h>
#include <etl/to_arithmetic.h>

#include "husarnet/logging.h"

size_t HTTPMessage::encode(etl::ivector<char>& buffer)
{
  buffer.clear();

  // Start line
  etl::array<char, 80> line;
  size_t len = std::snprintf(
      line.data(), line.size(), "%s %s HTTP/1.1\r\n",
      this->request.method.c_str(), this->request.endpoint.c_str());

  etl::copy(line.begin(), line.begin() + len, etl::back_inserter(buffer));

  // Headers
  for(auto& [key, value] : headers) {
    len = std::snprintf(
        line.data(), line.size(), "%s: %s\r\n", key.c_str(), value.c_str());
    etl::copy(line.begin(), line.begin() + len, etl::back_inserter(buffer));
  }

  // Header-body separator
  buffer.push_back('\r');
  buffer.push_back('\n');

  return buffer.size();
}

bool HTTPMessage::parse(etl::string_view& buffer_view, HTTPMessage& message)
{
  // TODO: Allow for chunked messages

  message.headers.clear();
  message.messageType = Type::UNDEFINED;

  // Find header-body separator
  size_t sep = buffer_view.find("\r\n\r\n");

  // Split by lines
  size_t pos = 0;
  while(pos < sep) {
    size_t end = buffer_view.find("\r\n", pos);
    if(end == std::string::npos)
      return false;

    etl::string_view line = buffer_view.substr(pos, end - pos);
    pos = end + 2;

    if(line.empty())
      break;

    if(message.messageType == Type::UNDEFINED) {
      // Try to parse start line
      etl::string_view field1, field2, field3;
      size_t space1 = line.find(' ');
      size_t space2 = line.find(' ', space1 + 1);

      if(space1 == etl::istring::npos || space2 == etl::istring::npos)
        return false;

      field1 = line.substr(0, space1);
      field2 = line.substr(space1 + 1, space2 - space1 - 1);
      field3 = line.substr(space2 + 1);

      if(field1.empty() || field2.empty() || field3.empty())
        return false;

      // Check if it's a request or response
      if(field1 == "HTTP/1.1")  // Response
      {
        message.messageType = HTTPMessage::Type::RESPONSE;

        auto statusCode = etl::to_arithmetic<unsigned int>(field2);
        if(!statusCode.has_value())
          return false;

        message.response.statusCode = statusCode.value();
        // message.http.response.statusMessage = field3;
      } else if(field3 == "HTTP/1.1")  // Request
      {
        message.messageType = HTTPMessage::Type::REQUEST;
        message.request.method.assign(field1.begin(), field1.end());
        message.request.endpoint.assign(field2.begin(), field2.end());

        if(message.request.method.is_truncated() ||
           message.request.endpoint.is_truncated())
          return false;
      } else {
        return false;
      }
    } else {
      // Parse headers
      size_t colon = line.find(':');
      if(colon == std::string::npos)
        return false;

      if(!message.headers.full()) {
        HeaderMap::key_type key(line.substr(0, colon));
        HeaderMap::mapped_type value(line.substr(colon + 2));

        message.headers.insert(etl::make_pair(key, value));
      } else {
        LOG_WARNING("HTTPMessage: too many headers");
      }
    }
  }

  return true;
}