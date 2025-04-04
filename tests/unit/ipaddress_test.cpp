// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ipaddress.h"

#include <catch2/catch_all.hpp>

TEST_CASE("ipaddress parse/stringify")
{
  REQUIRE(IpAddress::parse("127.0.0.1").toString() == "127.0.0.1");
  REQUIRE(IpAddress::parse("::").toString() == "0000:0000:0000:0000:0000:0000:0000:0000");
  REQUIRE(IpAddress::parse("123:4567::90").toString() == "0123:4567:0000:0000:0000:0000:0000:0090");
  REQUIRE(IpAddress::parse("1::1").toString() == "0001:0000:0000:0000:0000:0000:0000:0001");
  REQUIRE(IpAddress::parse("::ffff:5:6").toString() == "0.5.0.6");
  REQUIRE(IpAddress::parse("xxx").isInvalid());
}

TEST_CASE("ipaddress classify")
{
  REQUIRE(!IpAddress::parse("8.8.8.8").isPrivateV4());
  REQUIRE(IpAddress::parse("10.1.0.1").isPrivateV4());
  REQUIRE(IpAddress::parse("127.168.5.0").isPrivateV4());
  REQUIRE(!IpAddress::parse("192.167.0.0").isPrivateV4());
  REQUIRE(IpAddress::parse("192.168.5.0").isPrivateV4());
  REQUIRE(IpAddress::parse("172.16.5.0").isPrivateV4());
  REQUIRE(IpAddress::parse("172.31.5.0").isPrivateV4());
  REQUIRE(!IpAddress::parse("172.32.0.0").isPrivateV4());
}

TEST_CASE("inetaddress parse/stringify")
{
  REQUIRE(InetAddress::parse("127.0.0.1:80").str() == "127.0.0.1:80");
  REQUIRE(!InetAddress::parse("127.0.0.1"));
  REQUIRE(InetAddress::parse("[::1]:80").str() == "[0000:0000:0000:0000:0000:0000:0000:0001]:80");
  REQUIRE(InetAddress::parse("[127.0.0.1]:80").str() == "127.0.0.1:80");
}
