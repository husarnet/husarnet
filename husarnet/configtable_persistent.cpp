#include "configtable_persistent.h"
#include <sqlite3.h>
#include <exception>
#include <functional>
#include <vector>
#include "util.h"

void bindText(sqlite3_stmt* stmt, int i, const std::string& s) {
  int res =
      sqlite3_bind_text(stmt, i + 1, s.data(), s.size(), SQLITE_TRANSIENT);
  if (res != SQLITE_OK) {
    LOG("bind failed with %d", res);
    abort();
  }
}

void sqliteBind(sqlite3_stmt* stmt, std::vector<std::string> args) {
  sqlite3_reset(stmt);
  sqlite3_clear_bindings(stmt);
  for (int i = 0; i < args.size(); i++)
    bindText(stmt, i, args[i]);
}

std::string sqliteGetValue(sqlite3_stmt* stmt, int pos) {
  int length = sqlite3_column_bytes(stmt, pos);
  const uint8_t* data = sqlite3_column_text(stmt, pos);
  return std::string((const char*)data, length);
}

__attribute__((noreturn)) void sqliteError(sqlite3_stmt* stmt) {
  LOG("SQlite3 operation failed. Is your disk full or the database corrupted?");
  LOG("SQL: %s", sqlite3_sql(stmt));
  throw ConfigEditFailed("sqlite operation failed");
}

bool sqliteSingleRow(sqlite3_stmt* stmt) {
  int res = sqlite3_step(stmt);
  if (res == SQLITE_DONE)
    return false;
  if (res == SQLITE_ROW)
    return true;
  sqliteError(stmt);
}

template <typename T>
std::vector<T> sqliteIterate(sqlite3_stmt* stmt, std::function<T()> f) {
  std::vector<T> result;
  while (true) {
    int res = sqlite3_step(stmt);
    if (res == SQLITE_DONE) {
      break;
    } else if (res == SQLITE_ROW) {
      result.push_back(f());
    } else {
      sqliteError(stmt);
    }
  }
  return result;
}

struct SqliteConfigTable : ConfigTable {
  // This class is a thin wrapper over SQlite database.
  sqlite3* db = nullptr;

  std::string filename;

  sqlite3_stmt* beginTransaction = nullptr;
  sqlite3_stmt* endTransaction = nullptr;
  sqlite3_stmt* insertStmt = nullptr;
  sqlite3_stmt* selectTupleStmt = nullptr;
  sqlite3_stmt* deleteStmt = nullptr;
  sqlite3_stmt* getValueForNetworkStmt = nullptr;
  sqlite3_stmt* getValueStmt = nullptr;
  sqlite3_stmt* listNetworksStmt = nullptr;
  sqlite3_stmt* listValuesForNetworkStmt = nullptr;
  sqlite3_stmt* removeValuesStmt = nullptr;
  sqlite3_stmt* removeAllStmt = nullptr;

  sqlite3_stmt* prepareStmt(const char* stmt) {
    sqlite3_stmt* res;
    if (sqlite3_prepare_v2(db, stmt, -1, &res, 0) != SQLITE_OK) {
      LOG("cannot prepare statement %s", stmt);
      abort();
    }
    return res;
  }

  explicit SqliteConfigTable(std::string filename) {
    this->filename = filename;
  }

  void open() override {
    if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
      LOG("fail to open database");
      abort();
    }

    if (sqlite3_db_readonly(db, "main")) {
      LOG("error: database file %s is readonly!", filename.c_str());
    }

    beginTransaction = prepareStmt("begin transaction");
    endTransaction = prepareStmt("end transaction");

    auto tableExistsStmt = prepareStmt(
        "select name from sqlite_master where type='table' AND "
        "name='websetup_data'");
    sqliteBind(tableExistsStmt, {});
    if (!sqliteSingleRow(tableExistsStmt)) {
      LOG("creating new config database");
      std::vector<std::string> stmts = {
          "begin transaction;",
          "create table websetup_data (network text, kind text, key text, "
          "value text);",
          "create index websetup_data_key on websetup_data (kind, key);",
          "create index websetup_data_net on websetup_data (network, kind, "
          "key);",
          "create unique index websetup_data_uniq on websetup_data (network, "
          "kind, key, value);",
          "end transaction;"};
      for (std::string sql : stmts) {
        auto stmt = prepareStmt(sql.c_str());
        sqliteBind(stmt, {});
        sqliteSingleRow(stmt);
      }
    }

    insertStmt = prepareStmt(
        "insert into websetup_data (network, kind, key, value) values "
        "(?,?,?,?);");
    selectTupleStmt = prepareStmt(
        "select network,kind,key,value from websetup_data where network=? and "
        "kind=? and key=? and value=?;");
    deleteStmt = prepareStmt(
        "delete from websetup_data where network=? and kind=? and key=? and "
        "value=?;");
    removeValuesStmt = prepareStmt(
        "delete from websetup_data where network=? and kind=? and key=?");
    removeAllStmt =
        prepareStmt("delete from websetup_data where network=? and kind=?");
    getValueForNetworkStmt = prepareStmt(
        "select value from websetup_data where network=? and kind=? and "
        "key=?;");
    getValueStmt =
        prepareStmt("select value from websetup_data where kind=? and key=?;");
    listNetworksStmt =
        prepareStmt("select distinct network from websetup_data;");
    listValuesForNetworkStmt = prepareStmt(
        "select key,value from websetup_data where network=? and kind=?;");
  }

  void runInTransaction(std::function<void()> f) override {
    sqliteBind(beginTransaction, {});
    if (!sqlite3_step(beginTransaction))
      sqliteError(beginTransaction);

    // TODO: we might want to catch exceptions
    f();

    sqliteBind(endTransaction, {});
    if (!sqlite3_step(endTransaction))
      sqliteError(endTransaction);
  }

  bool containsRow(const ConfigRow& row) {
    sqliteBind(selectTupleStmt, {row.networkId, row.kind, row.key, row.value});
    int res = sqlite3_step(selectTupleStmt);
    if (res == SQLITE_DONE)
      return false;
    if (res == SQLITE_ROW)
      return true;
    sqliteError(selectTupleStmt);
  }

  bool insert(const ConfigRow& row) override {
    if (containsRow(row))
      return false;
    sqliteBind(insertStmt, {row.networkId, row.kind, row.key, row.value});
    if (sqlite3_step(insertStmt) != SQLITE_DONE)
      sqliteError(insertStmt);
    return true;
  }

  bool remove(const ConfigRow& row) override {
    if (!containsRow(row))
      return false;
    sqliteBind(deleteStmt, {row.networkId, row.kind, row.key, row.value});
    int res = sqlite3_step(deleteStmt);
    if (res != SQLITE_DONE)
      sqliteError(deleteStmt);
    return true;
  }

  void removeValues(const std::string& networkId,
                    const std::string& kind,
                    const std::string& key) override {
    sqliteBind(removeValuesStmt, {networkId, kind, key});
    int res = sqlite3_step(removeValuesStmt);
    if (res != SQLITE_DONE)
      sqliteError(removeValuesStmt);
  }

  void removeAll(const std::string& networkId,
                 const std::string& kind) override {
    sqliteBind(removeAllStmt, {networkId, kind});
    int res = sqlite3_step(removeAllStmt);
    if (res != SQLITE_DONE)
      sqliteError(removeAllStmt);
  }

  std::vector<std::string> getValueForNetwork(const std::string& networkId,
                                              const std::string& kind,
                                              const std::string& key) override {
    sqliteBind(getValueForNetworkStmt, {networkId, kind, key});
    return sqliteIterate<std::string>(getValueForNetworkStmt, [&]() {
      return sqliteGetValue(getValueForNetworkStmt, 0);
    });
  }

  std::vector<std::string> getValue(const std::string& kind,
                                    const std::string& key) override {
    sqliteBind(getValueStmt, {kind, key});
    return sqliteIterate<std::string>(
        getValueStmt, [&]() { return sqliteGetValue(getValueStmt, 0); });
  }

  std::vector<std::string> listNetworks() override {
    sqliteBind(listNetworksStmt, {});
    return sqliteIterate<std::string>(listNetworksStmt, [&]() {
      return sqliteGetValue(listNetworksStmt, 0);
    });
  }

  std::vector<ConfigRow> listValuesForNetwork(
      const std::string& networkId,
      const std::string& kind) override {
    sqliteBind(listValuesForNetworkStmt, {networkId, kind});
    return sqliteIterate<ConfigRow>(listValuesForNetworkStmt, [&]() {
      return ConfigRow{networkId, kind,
                       sqliteGetValue(listValuesForNetworkStmt, 0),
                       sqliteGetValue(listValuesForNetworkStmt, 1)};
    });
  }
};

ConfigTable* createSqliteConfigTable(std::string filename) {
  return new SqliteConfigTable(filename);
}
