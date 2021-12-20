#pragma once
#include <functional>
#include <stdexcept>
#include "util.h"

struct ConfigRow {
  std::string networkId;
  std::string kind;
  std::string key;
  std::string value;
};

struct ConfigEditFailed : std::runtime_error {
  explicit ConfigEditFailed(std::string msg) : std::runtime_error(msg) {}
};

struct ConfigTable {
  virtual void runInTransaction(std::function<void()> f) = 0;

  // Insert a row into the table. Return true if it wasn't present before.
  virtual bool insert(const ConfigRow& row) = 0;

  // Remove a row from the table. Return true if it was present before.
  virtual bool remove(const ConfigRow& row) = 0;

  // Replace all values bound to a specific key with `newValue`.
  virtual void removeValues(const std::string& networkId,
                            const std::string& kind,
                            const std::string& key) = 0;

  // Replace all values bound of a specific kind in a given network.
  virtual void removeAll(const std::string& networkId,
                         const std::string& kind) = 0;

  // Return values corresponding to the rest of the tuple given in the
  // arguments.
  virtual std::vector<std::string> getValueForNetwork(
      const std::string& networkId,
      const std::string& kind,
      const std::string& key) = 0;

  // Same as getValueForNetwork, but check values for all networks.
  virtual std::vector<std::string> getValue(const std::string& kind,
                                            const std::string& key) = 0;

  virtual std::vector<std::string> listNetworks() = 0;
  virtual std::vector<ConfigRow> listValuesForNetwork(
      const std::string& networkId,
      const std::string& kind) = 0;

  virtual void open() = 0;

  virtual ~ConfigTable() {}
};
