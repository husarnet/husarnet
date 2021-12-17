#include "catch.hpp"
#include "memory_configtable.h"
#include "sqlite_configtable.h"

template <typename T>
std::vector<T> sorted(std::vector<T> v) {
  std::sort(v.begin(), v.end());
  return v;
}

void testConfigTable(ConfigTable* t) {
  t->insert(ConfigRow{"testnet", "testcat", "testkey", "testval"});
  REQUIRE(t->getValue("testcat", "testkey") ==
          std::vector<std::string>{"testval"});

  t->insert(ConfigRow{"testnet", "testcat", "testkey", "testval2"});
  REQUIRE(sorted(t->getValue("testcat", "testkey")) ==
          std::vector<std::string>{"testval", "testval2"});
  ;

  t->remove(ConfigRow{"testnet", "testcat", "testkey", "testval"});
  REQUIRE(t->getValue("testcat", "testkey") ==
          std::vector<std::string>{"testval2"});
  ;

  t->insert(ConfigRow{"testnet1", "testcat", "testkey", "testval3"});
  REQUIRE(t->getValueForNetwork("testnet", "testcat", "testkey") ==
          std::vector<std::string>{"testval2"});
  ;
  REQUIRE(sorted(t->getValue("testcat", "testkey")) ==
          std::vector<std::string>{"testval2", "testval3"});
  REQUIRE(sorted(t->listNetworks()) ==
          std::vector<std::string>{"testnet", "testnet1"});

  t->removeAll("testnet", "testcat");
  REQUIRE(t->getValue("testcat", "testkey") ==
          std::vector<std::string>{"testval3"});
  REQUIRE(t->listNetworks() == std::vector<std::string>{"testnet1"});
}

TEST_CASE("SQlite simple tests") {
  ConfigTable* t = createSqliteConfigTable(":memory:");
  t->open();
  testConfigTable(t);
}

TEST_CASE("memory configtable simple tests") {
  ConfigTable* t = createMemoryConfigTable("", [](std::string data) {});
  t->open();
  testConfigTable(t);
}
