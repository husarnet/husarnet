// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/config_storage.h"

#include <catch2/catch.hpp>

#include "husarnet/husarnet_manager.h"

static ConfigStorage* makeTestStorage()
{
    HusarnetManager* manager = new HusarnetManager();
    auto cs = new ConfigStorage(
              manager, []() { return ""; }, [&](std::string s) {}, {}, {}, {});

    manager->setConfigStorage(cs);
    return cs;
}

TEST_CASE("Group changes")
{
  int writes = 0;
  auto cs = new ConfigStorage(
      new HusarnetManager(), []() { return ""; },
      [&](std::string s) { writes++; }, {}, {}, {});

  // TODO long term - re add this
  // cs->hostTableAdd("foo", IpAddress::parse("dead:beef::1"));
  // cs->hostTableAdd("bar", IpAddress::parse("dead:beef::2"));
  // cs->hostTableAdd("baz", IpAddress::parse("dead:beef::3"));

  // REQUIRE(writes == 3);

  // cs->groupChanges([&]() {
  //   cs->hostTableRm("foo");
  //   cs->hostTableRm("bar");
  //   cs->hostTableRm("baz");
  // });

  // REQUIRE(writes == 4);
}

TEST_CASE("ConfigStorage initialization")
{
  makeTestStorage();
}

TEST_CASE("Host table operations")
{
  auto cs = makeTestStorage();
  REQUIRE(cs->getHostTable().empty());

  std::string foo_hostname = "foo";
  auto foo_ip = IpAddress::parse("dead:beef::1");

  cs->hostTableAdd(foo_hostname, foo_ip);
  REQUIRE(cs->getHostTable().size() == 1);
  REQUIRE(cs->getHostTable().count(foo_hostname) == 1);
  REQUIRE(cs->getHostTable()[foo_hostname] == foo_ip);

  std::string bar_hostname = "bar";
  auto bar_ip = IpAddress::parse("dead:beef::2");

  cs->hostTableAdd(bar_hostname, bar_ip);
  REQUIRE(cs->getHostTable().size() == 2);
  REQUIRE(cs->getHostTable().count(bar_hostname) == 1);
  REQUIRE(cs->getHostTable()[bar_hostname] == bar_ip);

  cs->hostTableRm(bar_hostname);
  REQUIRE(cs->getHostTable().size() == 1);
  REQUIRE(cs->getHostTable().count(bar_hostname) == 0);

  cs->hostTableClear();
  REQUIRE(cs->getHostTable().empty());
}

TEST_CASE("Whitelist operations")
{
  auto cs = makeTestStorage();
  REQUIRE(cs->getWhitelist().empty());

  REQUIRE_FALSE(cs->isOnWhitelist(IpAddress::parse("dead:beef::0")));

  cs->whitelistAdd(IpAddress::parse("dead:beef::1"));
  REQUIRE(cs->isOnWhitelist(IpAddress::parse("dead:beef::1")));

  cs->whitelistAdd(IpAddress::parse("dead:beef::2"));
  REQUIRE(cs->isOnWhitelist(IpAddress::parse("dead:beef::2")));

  cs->whitelistRm(IpAddress::parse("dead:beef::1"));
  REQUIRE_FALSE(cs->isOnWhitelist(IpAddress::parse("dead:beef::1")));
  REQUIRE(cs->isOnWhitelist(IpAddress::parse("dead:beef::2")));

  cs->whitelistClear();
  REQUIRE_FALSE(cs->isOnWhitelist(IpAddress::parse("dead:beef::2")));
  REQUIRE(cs->getWhitelist().empty());
}

TEST_CASE("Internal settings")
{
  auto cs = makeTestStorage();

  SECTION("Unset fields should be empty and not error out")
  {
    CAPTURE(cs->getCurrentData());
    REQUIRE(cs->getInternalSetting(InternalSetting::websetupSecret) == "");
  }

  SECTION("Direct setting")
  {
    // confusing problem - string literal gets treated as bool
    // hence static_cast
    cs->setInternalSetting(InternalSetting::websetupSecret, static_cast<std::string>("foo"));
    REQUIRE(cs->getInternalSetting(InternalSetting::websetupSecret) == "foo");
  }

  HusarnetManager* manager = new HusarnetManager();
  cs = new ConfigStorage(
      manager, []() { return ""; }, [&](std::string s) {}, {}, {},
      {{InternalSetting::websetupSecret, "foo"}});
  manager->setConfigStorage(cs);

  SECTION("Default setting")
  {
    REQUIRE(cs->getInternalSetting(InternalSetting::websetupSecret) == "foo");
  }

  SECTION("Default setting override")
  {
    cs->setInternalSetting(InternalSetting::websetupSecret, static_cast<std::string>("bar"));
    REQUIRE(cs->getInternalSetting(InternalSetting::websetupSecret) == "bar");
  }
}

TEST_CASE("User settings")
{
  auto cs = makeTestStorage();

  SECTION("Unset fields should be empty and not error out")
  {
    CAPTURE(cs->getCurrentData());
    REQUIRE(cs->getUserSetting(UserSetting::dashboardFqdn) == "");
  }

  SECTION("Direct setting")
  {
    cs->setUserSetting(UserSetting::dashboardFqdn, static_cast<std::string>("foo"));
    REQUIRE(cs->getUserSetting(UserSetting::dashboardFqdn) == "foo");
  }

  HusarnetManager* manager = new HusarnetManager();
  cs = new ConfigStorage(
      manager, []() { return ""; }, [&](std::string s) {}, 
      {{UserSetting::dashboardFqdn, "foo"}}, {}, {});
  manager->setConfigStorage(cs);

  SECTION("Default setting")
  {
    REQUIRE(cs->getUserSetting(UserSetting::dashboardFqdn) == "foo");
  }

  SECTION("Default setting override")
  {
    cs->setUserSetting(UserSetting::dashboardFqdn, static_cast<std::string>("bar"));
    REQUIRE(cs->getUserSetting(UserSetting::dashboardFqdn) == "bar");
  }

  cs = new ConfigStorage(
      manager, []() { return ""; }, [&](std::string s) {},
      {{UserSetting::dashboardFqdn, "foo"}},
      {{UserSetting::dashboardFqdn, "bar"}}, {});
  manager->setConfigStorage(cs);

  SECTION("Override setting")
  {
    REQUIRE(cs->getUserSetting(UserSetting::dashboardFqdn) == "bar");
  }

  SECTION("Override still wins")
  {
    cs->setUserSetting(UserSetting::dashboardFqdn, "baz");
    REQUIRE(cs->getUserSetting(UserSetting::dashboardFqdn) == "bar");
  }
}
