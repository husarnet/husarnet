// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "util.h"
#include <catch2/catch.hpp>

TEST_CASE("split whitespace") {
  REQUIRE(splitWhitespace("") == std::vector<std::string>{});

  REQUIRE(splitWhitespace("x") == std::vector<std::string>{"x"});
  REQUIRE(splitWhitespace("a b cc  d ") ==
          std::vector<std::string>{"a", "b", "cc", "d"});
}

TEST_CASE("split") {
  REQUIRE(split("ax=b", '=', 1) == std::vector<std::string>{"ax", "b"});
  REQUIRE(split("a=b=c", '=', 1) == std::vector<std::string>{"a", "b=c"});
  REQUIRE(split("a==b=c", '=', 1) == std::vector<std::string>{"a", "=b=c"});
  REQUIRE(split("a=", '=', 1) == std::vector<std::string>{"a", ""});
  REQUIRE(split("asad", '=', 1) == std::vector<std::string>{"asad"});
}

TEST_CASE("parse int") {
  REQUIRE(parse_integer("1") == std::make_pair(true, 1));
  REQUIRE(parse_integer("-1") == std::make_pair(true, -1));
  REQUIRE(parse_integer("--1") == std::make_pair(false, -1));
  REQUIRE(parse_integer("1aa") == std::make_pair(false, -1));
  REQUIRE(parse_integer("aa") == std::make_pair(false, -1));
  REQUIRE(parse_integer("") == std::make_pair(false, -1));
  REQUIRE(parse_integer("0") == std::make_pair(true, 0));
  REQUIRE(parse_integer("12") == std::make_pair(true, 12));
  REQUIRE(parse_integer("2147483648") == std::make_pair(false, -1));
  REQUIRE(parse_integer("1000000000") == std::make_pair(true, 1000000000));
  REQUIRE(parse_integer("-1000000009") == std::make_pair(true, -1000000009));
  REQUIRE(parse_integer("10000000000") == std::make_pair(false, -1));
  REQUIRE(parse_integer("-2147483648") ==
          std::make_pair(true, (int)-2147483648));
}

TEST_CASE("base64") {
  REQUIRE(
      base64Decode("/JQAAAAAAAAAAAAAAAAAAA") ==
      std::string(
          "\xfc\x94\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
          16));
  REQUIRE(base64Decode("s6P0S2M") == ("\xb3\xa3\xf4"
                                      "Kc"));
  REQUIRE(base64Decode("s6P0S2M=") == ("\xb3\xa3\xf4"
                                       "Kc"));
}
