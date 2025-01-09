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

  // Start line ({method} {endpoint} HTTP/1.1)
  buffer.insert(
      buffer.end(), this->request.method.begin(), this->request.method.end());
  buffer.push_back(' ');
  buffer.insert(
      buffer.end(), this->request.endpoint.begin(),
      this->request.endpoint.end());
  etl::string<11> version = " HTTP/1.1\r\n";
  buffer.insert(buffer.end(), version.begin(), version.end());

  if(!this->body.empty()) {
    this->headers["Content-Length"] = std::to_string(this->body.size());
  }

  // Headers ({key}: {value})
  for(auto& [key, value] : headers) {
    buffer.insert(buffer.end(), key.begin(), key.end());
    buffer.push_back(':');
    buffer.push_back(' ');
    buffer.insert(buffer.end(), value.begin(), value.end());
    buffer.insert(buffer.end(), EOL_DELIMITER.begin(), EOL_DELIMITER.end());
  }

  // Header-body separator (double EOL_DELIMITER)
  buffer.insert(buffer.end(), EOL_DELIMITER.begin(), EOL_DELIMITER.end());

  // Body
  buffer.insert(buffer.end(), this->body.begin(), this->body.end());

  return buffer.size();
}

HTTPMessage::Result HTTPMessage::_parseStartLine()
{
  etl::string_view buffer_view(
      &(*this->_it), std::distance(this->_it, this->_buffer.end()));

  // Find end of line
  size_t end = buffer_view.find(EOL_DELIMITER);
  if(end == etl::string_view::npos)
    return Result::INCOMPLETE;

  // Split start line by spaces
  size_t space1 = buffer_view.find(' ');
  size_t space2 = buffer_view.find(' ', space1 + 1);
  if(space1 == etl::string_view::npos || space2 == etl::string_view::npos)
    return Result::INVALID;

  etl::string_view field1 = buffer_view.substr(0, space1);
  etl::string_view field2 = buffer_view.substr(space1 + 1, space2 - space1 - 1);
  etl::string_view field3 = buffer_view.substr(space2 + 1, end - space2 - 1);

  if(field1.empty() || field2.empty() || field3.empty())
    return Result::INVALID;

  // Check if it's a request or response
  if(field1 == "HTTP/1.1")  // Response
  {
    this->messageType = Type::RESPONSE;

    auto statusCode = etl::to_arithmetic<unsigned int>(field2);
    if(!statusCode.has_value())
      return Result::INVALID;

    this->response.statusCode = statusCode.value();
    // this->http.response.statusMessage = field3;
  } else if(field3 == "HTTP/1.1")  // Request
  {
    this->messageType = Type::REQUEST;
    this->request.method.assign(field1.begin(), field1.end());
    this->request.endpoint.assign(field2.begin(), field2.end());

    // Verify fields
    if(this->request.method.empty() || this->request.endpoint.empty())
      return Result::INVALID;

    if(this->request.method.is_truncated() ||
       this->request.endpoint.is_truncated())
      return Result::INVALID;
  } else {
    return Result::INVALID;
  }

  // Advance iterator to header section
  this->_it += end + EOL_DELIMITER.size();

  return Result::OK;
}

HTTPMessage::Result HTTPMessage::_parseHeader()
{
  etl::string_view buffer_view(
      &(*this->_it), std::distance(this->_it, this->_buffer.end()));

  // Find header-body separator
  // TODO: something is fucked up here
  // double checking? does startline set _it in correct offset?
  size_t sep = buffer_view.find(BODY_DELIMITER);
  if(sep == etl::string_view::npos)
    return Result::INCOMPLETE;

  // Split by lines
  size_t pos = 0;
  while(pos < sep) {
    size_t end = buffer_view.find(EOL_DELIMITER, pos);
    if(end == etl::string_view::npos)
      return Result::INVALID;

    etl::string_view line = buffer_view.substr(pos, end - pos);
    pos = end + 2;

    if(line.empty())
      break;

    // Parse headers
    size_t colon = line.find(':');
    if(colon == etl::string_view::npos)
      return Result::INVALID;

    // Insert key-value pair into the headers map
    etl::string_view key_view = line.substr(0, colon);
    std::string key(key_view.data(), key_view.size());

    etl::string_view value_view = line.substr(colon + 2);
    std::string value(value_view.data(), value_view.size());

    this->headers.insert_or_assign(key, value);
  }

  // Advance iterator to body section
  this->_it += sep + BODY_DELIMITER.size();

  this->_parserState = ParseState::BODY;
  return Result::OK;
}

HTTPMessage::Result HTTPMessage::_parseBody()
{
  etl::string_view buffer_view(
      &(*this->_it), std::distance(this->_it, this->_buffer.end()));

  auto it = this->headers.find("Content-Length");

  // Message does not contain explicit body length
  // Assume it has no body
  if(it == this->headers.end())
    return Result::OK;

  size_t contentLength =
      etl::to_arithmetic<size_t>(etl::string_view(it->second.data()));

  if(buffer_view.size() < contentLength)
    return Result::INCOMPLETE;

  if(contentLength == 0) {
    this->body.assign(nullptr, nullptr);
  } else {
    this->body.assign(&(*this->_it), &(*(this->_it + contentLength)));
  }

  return Result::OK;
}

// Parses HTTP message from provided data chunks
// Returns OK if the message is complete and valid
// If called after successful parsing, previous result will be invalidated
// Calling this function with pending chunks will return INCOMPLETE
HTTPMessage::Result HTTPMessage::parse(etl::string_view& buffer_view)
{
  // Enforce maximum message size limit
  size_t newSize = this->_buffer.size() + buffer_view.size();
  if(newSize > MAX_MESSAGE_LENGTH) {
    this->_buffer.clear();
    return Result::INVALID;
    ;
  }

  // Parsing is done on a dynamic internal buffer
  // to provide support for chunked messages
  this->_buffer.insert(
      this->_buffer.end(), buffer_view.begin(), buffer_view.end());

  Result res = Result::OK;

  // Try to parse buffer until a complete message is found or an error occurs
  while(res == Result::OK && this->_parserState != ParseState::DONE) {
    switch(this->_parserState) {
      case ParseState::START_LINE:
        // Erase previous message
        if(this->_it != this->_buffer.begin()) {
          this->_buffer.erase(this->_buffer.begin(), this->_it);
          this->_it = this->_buffer.begin();
        }

        res = this->_parseStartLine();

        if(res == Result::OK)
          this->_parserState = ParseState::HEADER;
        break;

      case ParseState::HEADER:
        res = this->_parseHeader();

        if(res == Result::OK)
          this->_parserState = ParseState::BODY;
        break;

      case ParseState::BODY:
        res = this->_parseBody();

        if(res == Result::OK)
          this->_parserState = ParseState::DONE;
        break;

      case ParseState::DONE:
        return Result::INVALID;
    }
  }

  // Reset parser state if parsing has ended
  if(res == Result::OK || res == Result::INVALID) {
    this->_parserState = ParseState::START_LINE;
  }

  // Invalidate entire buffer on error
  if(res == Result::INVALID) {
    this->_buffer.clear();
    this->_it = this->_buffer.begin();
  }

  return res;
}
