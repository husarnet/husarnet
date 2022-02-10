#include "config_storage.h"

#define HOST_TABLE_KEY "host-table"

ConfigStorage::ConfigStorage(
    std::function<std::string()> readFunc,
    std::function<void(std::string)> writeFunc,
    std::map<UserSetting, std::string> settingsDefaults,
    std::map<UserSetting, std::string> settingsEnvOverrides)
    : readFunc(readFunc),
      writeFunc(writeFunc),
      settingsDefaults(settingsDefaults),
      settingsEnvOverrides(settingsEnvOverrides) {
  deserialize(readFunc());
}

std::string ConfigStorage::serialize() {
  return this->currentData.dump(4);
}

void ConfigStorage::deserialize(std::string blob) {
  this->currentData = json::parse(blob);
}

void ConfigStorage::save() {
  if (!this->shouldSaveImmediately) {
    return;
  }

  writeFunc(serialize());
}

void ConfigStorage::groupChanges(std::function<void()> f) {
  this->shouldSaveImmediately = false;
  f();
  this->shouldSaveImmediately = true;
  save();
}

void hostTableAdd(std::string hostname, IpAddress address) {
  //   currentData[HOST_TABLE_KEY].
}

// #include "configstorage.h"
// #include <exception>
// #include <functional>
// #include <unordered_map>
// #include <unordered_set>
// #include <vector>
// #include "util.h"

// struct MemoryConfigTable : ConfigTable {
//   struct Key {
//     std::string kind;
//     std::string key;

//     bool operator==(const Key& o) const {
//       return kind == o.kind && key == o.key;
//     }
//   };

//   struct KeyHash {
//     unsigned operator()(const Key& k) const {
//       return pair_hash<std::string, std::string>()(
//           std::make_pair(k.kind, k.key));
//     }
//   };

//   struct Value {
//     std::string networkId;
//     std::string value;

//     bool operator==(const Value& o) const {
//       return networkId == o.networkId && value == o.value;
//     }
//   };

//   struct ValueHash {
//     unsigned operator()(const Value& k) const {
//       return pair_hash<std::string, std::string>()(
//           std::make_pair(k.value, k.networkId));
//     }
//   };

//   std::unordered_map<Key, std::unordered_set<Value, ValueHash>, KeyHash>
//   values;

//   // serialization

//   static void stripZeroBytes(std::string s) {
//     // there should be no zero bytes in the data, but better be sure
//     for (char& c : s)
//       if (c == '\0')
//         c = ' ';
//   }

//   std::string serialize() {
//     std::string result;
//     for (const auto& vals : values) {
//       Key k = vals.first;
//       stripZeroBytes(k.key);
//       stripZeroBytes(k.kind);
//       for (Value v : vals.second) {
//         stripZeroBytes(v.networkId);
//         stripZeroBytes(v.value);
//         result +=
//             v.networkId + '\0' + k.kind + '\0' + k.key + '\0' + v.value +
//             '\0';
//       }
//     }
//     return result;
//   }

//   void deserialize(std::string data) {
//     std::vector<std::string> split_data = split(data, '\0', 1000000);

//     for (int i = 0; i + 4 <= (int)split_data.size(); i += 4) {
//       insert(ConfigRow{split_data[i + 0], split_data[i + 1], split_data[i
//       + 2],
//                        split_data[i + 3]});
//     }
//   }

//   std::string lastWrittenData;
//   std::function<void(std::string)> writeFunc;

//   void writeData() {
//     std::string serialized = serialize();
//     if (serialized != lastWrittenData) {
//       writeFunc(serialized);
//       lastWrittenData = serialized;
//     }
//   }

//   // -----

//   void runInTransaction(std::function<void()> f) override {
//     f();
//     writeData();
//   }

//   static Key _key(const ConfigRow& row) { return Key{row.kind, row.key};
//   } static Value _value(const ConfigRow& row) {
//     return Value{row.networkId, row.value};
//   }

//   bool containsRow(const ConfigRow& row) {
//     auto it = values.find(_key(row));
//     if (it == values.end())
//       return false;
//     return it->second.find(_value(row)) != it->second.end();
//   }

//   bool insert(const ConfigRow& row) override {
//     return values[_key(row)].insert(_value(row)).second;
//   }

//   bool remove(const ConfigRow& row) override {
//     return values[_key(row)].erase(_value(row));
//   }

//   void removeValues(const std::string& networkId,
//                     const std::string& kind,
//                     const std::string& key) override {
//     auto& coll = values[Key{kind, key}];
//     auto it = coll.begin();
//     while (it != coll.end()) {
//       if (it->networkId == networkId) {
//         auto toErase = it;
//         it++;
//         coll.erase(toErase);
//       } else
//         it++;
//     }
//   }

//   void removeAll(const std::string& networkId,
//                  const std::string& kind) override {
//     for (auto& p : values) {
//       const Key& key = p.first;
//       if (key.kind != kind)
//         continue;

//       auto& coll = p.second;
//       auto it = coll.begin();
//       while (it != coll.end()) {
//         if (it->networkId == networkId) {
//           auto toErase = it;
//           it++;
//           coll.erase(toErase);
//         } else
//           it++;
//       }
//     }
//   }

//   std::vector<std::string> getValueForNetwork(const std::string&
//   networkId,
//                                               const std::string& kind,
//                                               const std::string& key)
//                                               override {
//     std::vector<std::string> result;
//     for (const Value& v : values[Key{kind, key}]) {
//       if (v.networkId == networkId)
//         result.push_back(v.value);
//     }
//     return result;
//   }

//   std::vector<std::string> getValue(const std::string& kind,
//                                     const std::string& key) override {
//     std::vector<std::string> result;
//     auto it = values.find(Key{kind, key});
//     if (it != values.end()) {
//       for (const Value& v : it->second) {
//         result.push_back(v.value);
//       }
//     }
//     return result;
//   }

//   std::vector<std::string> listNetworks() override {
//     std::unordered_set<std::string> result;
//     for (const auto& p : values) {
//       for (const Value& v : p.second) {
//         result.insert(v.networkId);
//       }
//     }
//     return std::vector<std::string>(result.begin(), result.end());
//   }

//   std::vector<ConfigRow> listValuesForNetwork(
//       const std::string& networkId,
//       const std::string& kind) override {
//     std::vector<ConfigRow> result;
//     for (const auto& p : values) {
//       const Key& k = p.first;
//       for (const Value& v : p.second) {
//         if (v.networkId == networkId && k.kind == kind) {
//           ConfigRow r;
//           r.key = k.key;
//           r.kind = k.kind;
//           r.networkId = v.networkId;
//           r.value = v.value;
//           result.push_back(r);
//         }
//       }
//     }
//     return result;
//   }
// };
