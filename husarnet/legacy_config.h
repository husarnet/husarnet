// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>
#include <functional>

class LegacyConfig {
 private:
   sqlite3* db;
   std::string pathToLegacyConfig;
   bool valid;

   // we only need some sqlite wrapper functions,
   // keep them private
   sqlite3_stmt* sqlitePrepareStatement(const char* stmt);
   void sqliteBind(sqlite3_stmt* stmt, std::vector<std::string> args);
   std::vector<std::string> sqliteIterate(sqlite3_stmt* stmt, std::function<std::string()> f);
   std::string sqliteGetValue(sqlite3_stmt* stmt, int pos);

 public:
  LegacyConfig(std::string configPath) : db(nullptr), pathToLegacyConfig(configPath), valid(false) {}

  void open();
  bool isValid();

  std::string getWebsetupSecret();
  bool getWhitelistEnabled();
  std::vector<std::string> getWhitelistEntries();
};
