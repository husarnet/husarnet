// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <etl/map.h>
#include <etl/string.h>
#include <etl/vector.h>

class HTTPMessage {
 public:
  HTTPMessage(){};

  static bool parse(etl::ivector<char>& buffer, HTTPMessage& message);
  // TODO: overflow check?
  size_t encode(etl::ivector<char>& buffer);

  // union http {
  //   http() {};
  //   ~http() {};

  //   struct request {
  //     request(): method(), endpoint() {};
  //     etl::string<7> method;
  //     etl::string<32> endpoint;
  //   } request;

  //   struct response {
  //     response() {};
  //     unsigned int statusCode = 0;
  //   } response;
  // } http;

  struct {
    unsigned int statusCode = 0;
  } response;

  struct request {
    etl::string<7> method;
    etl::string<32> endpoint;
  } request;

  enum class Type
  {
    UNDEFINED = 0,
    REQUEST,
    RESPONSE,
  };

  Type messageType = Type::UNDEFINED;

  // TODO: approach to this, maybe use static buf with stringviews
  // maybe stl::unordered_map with static alloc
  // anyway, having static allocator to use in stl containers would be nice
  using HeaderMap = etl::map<etl::string<32>, etl::string<32>, 16>;
  HeaderMap headers;
};