// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/util.h"

#include <catch2/catch.hpp>

TEST_CASE("split whitespace")
{
  REQUIRE(splitWhitespace("") == std::vector<std::string>{});

  REQUIRE(splitWhitespace("x") == std::vector<std::string>{"x"});
  REQUIRE(
      splitWhitespace("a b cc  d ") ==
      std::vector<std::string>{"a", "b", "cc", "d"});
}

TEST_CASE("split")
{
  REQUIRE(split("ax=b", '=', 1) == std::vector<std::string>{"ax", "b"});
  REQUIRE(split("a=b=c", '=', 1) == std::vector<std::string>{"a", "b=c"});
  REQUIRE(split("a==b=c", '=', 1) == std::vector<std::string>{"a", "=b=c"});
  REQUIRE(split("a=", '=', 1) == std::vector<std::string>{"a", ""});
  REQUIRE(split("asad", '=', 1) == std::vector<std::string>{"asad"});
}

TEST_CASE("base64")
{
  REQUIRE(
      base64Decode("/JQAAAAAAAAAAAAAAAAAAA") ==
      std::string(
          "\xfc\x94\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
          16));
  REQUIRE(
      base64Decode("s6P0S2M") == ("\xb3\xa3\xf4"
                                  "Kc"));
  REQUIRE(
      base64Decode("s6P0S2M=") == ("\xb3\xa3\xf4"
                                   "Kc"));
}

TEST_CASE("mapContains")
{
  std::map<const char*, int> m = {{"a", 1}, {"b", 2}, {"c", 3}};
  REQUIRE(mapContains(m, "a"));
  REQUIRE(mapContains(m, "b"));
  REQUIRE(mapContains(m, "c"));
  REQUIRE(!mapContains(m, "d"));
}

TEST_CASE("mapContains empty map")
{
  std::map<const char*, int> m;
  REQUIRE(!mapContains(m, "a"));
}
