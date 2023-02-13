// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/legacy_config.h"

#include <sqlite3.h>

#include "husarnet/ports/port_interface.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

sqlite3_stmt* LegacyConfig::sqlitePrepareStatement(const char* stmt)
{
  sqlite3_stmt* res;
  if(sqlite3_prepare_v2(db, stmt, -1, &res, 0) != SQLITE_OK) {
    LOG_WARNING("cannot prepare SQLite statement %s", stmt);
  }
  return res;
}

void LegacyConfig::sqliteBind(sqlite3_stmt* stmt, std::vector<std::string> args)
{
  sqlite3_reset(stmt);
  sqlite3_clear_bindings(stmt);
  for(int i = 0; i < args.size(); i++) {
    int res = sqlite3_bind_text(
        stmt, i + 1, args[i].data(), args[i].size(), SQLITE_TRANSIENT);
    if(res != SQLITE_OK) {
      LOG_WARNING("SQLite bind failed with %d", res);
    }
  }
}

std::vector<std::string> LegacyConfig::sqliteIterate(
    sqlite3_stmt* stmt,
    std::function<std::string()> f)
{
  std::vector<std::string> result;
  while(true) {
    int res = sqlite3_step(stmt);
    if(res == SQLITE_DONE) {
      break;
    } else if(res == SQLITE_ROW) {
      result.push_back(f());
    } else {
      LOG_WARNING("SQLite read failed");
    }
  }
  return result;
}

std::string LegacyConfig::sqliteGetValue(sqlite3_stmt* stmt, int pos)
{
  int length = sqlite3_column_bytes(stmt, pos);
  const uint8_t* data = sqlite3_column_text(stmt, pos);
  return std::string((const char*)data, length);
}

bool LegacyConfig::open()
{
  if(sqlite3_open(pathToLegacyConfig.c_str(), &db) != SQLITE_OK) {
    LOG_WARNING("failed to open legacy config database");
    return false;
  }

  return true;
}

std::string LegacyConfig::getWebsetupSecret()
{
  auto getWebsetupSecretStatement = sqlitePrepareStatement(
      "select value from websetup_data where kind=? and key=?;");
  sqliteBind(getWebsetupSecretStatement, {"config", "websetup-secret"});

  auto values = sqliteIterate(getWebsetupSecretStatement, [&]() {
    return sqliteGetValue(getWebsetupSecretStatement, 0);
  });
  return (values.size() > 0) ? values.at(0) : "";
}

bool LegacyConfig::getWhitelistEnabled()
{
  auto getWhitelistEnabledStatement = sqlitePrepareStatement(
      "select value from websetup_data where kind=? and key=?;");
  sqliteBind(getWhitelistEnabledStatement, {"config", "whitelist-enabled"});

  auto values = sqliteIterate(getWhitelistEnabledStatement, [&]() {
    return sqliteGetValue(getWhitelistEnabledStatement, 0);
  });

  if(values.size() > 0 && values.at(0) == "false") {
    return false;
  } else {
    return true;
  }
}

std::vector<std::string> LegacyConfig::getWhitelistEntries()
{
  auto getWhitelistEntriesStatement =
      sqlitePrepareStatement("select key from websetup_data where kind=?;");
  sqliteBind(getWhitelistEntriesStatement, {"whitelist"});

  auto values = sqliteIterate(getWhitelistEntriesStatement, [&]() {
    return sqliteGetValue(getWhitelistEntriesStatement, 0);
  });

  return values;
}
