// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/variant.h>
#include <etl/vector.h>

class HTTPMessage {
 public:
  enum class Result
  {
    OK = 0,
    INCOMPLETE,
    INVALID,
  };

  enum class Type
  {
    UNDEFINED = 0,
    REQUEST,
    RESPONSE,
  };

  enum class ParseState
  {
    START_LINE = 0,
    HEADER,
    BODY,
    DONE,
  };

  HTTPMessage()
  {
    this->_buffer.reserve(MAX_MESSAGE_LENGTH / 4);
    this->_it = this->_buffer.begin();
  };

  Result parse(etl::string_view& buffer_view);

  size_t encode(etl::ivector<char>& buffer);

  struct {
    unsigned int statusCode = 0;
  } response;

  struct {
    etl::string<7> method;
    etl::string<256> endpoint;
  } request;

  Type messageType = Type::UNDEFINED;

  using HeaderMap = std::map<std::string, std::string>;
  HeaderMap headers;

  // Contains message body
  // In case of request, it should be assigned to the body of the request
  // In case of response, it will contain a view into built-in buffer
  etl::string_view body;

 private:
  // etl::variant<etl::ivector<char>, std::vector<char>> _buffer;
  // TODO: etl with static alloc
  std::vector<char> _buffer;
  std::vector<char>::iterator _it;

  ParseState _parserState = ParseState::START_LINE;

  const etl::string<2> EOL_DELIMITER = "\r\n";
  const etl::string<4> BODY_DELIMITER = "\r\n\r\n";

  // TODO: make platform-specific
  const size_t MAX_MESSAGE_LENGTH = 8192;

  Result _parseStartLine();
  Result _parseHeader();
  Result _parseBody();
};
