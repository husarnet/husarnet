#include "legacy_filestorage.h"
#include <fstream>
#include "husarnet_crypto.h"
#include "port.h"
#include "service_helper.h"
#include "util.h"

namespace LegacyFileStorage {

std::ifstream openFile(std::string name) {
  std::ifstream f(name);
  if (!f.good()) {
    LOG("failed to open %s", name.c_str());
  }
  return f;
}

struct Whitelist {
  std::unordered_set<DeviceId> devices;
  bool isEnabled;
};

struct WebsetupConfig {
  std::string websetupSecret;
  std::vector<IpAddress> managerWhitelist;
};

class ConfigStorage {
 public:
  virtual Whitelist readWhitelist() = 0;
  virtual void writeWhitelist(Whitelist w) = 0;
  virtual WebsetupConfig readWebsetupConfig() = 0;
  virtual bool writeWebsetupConfig(WebsetupConfig config) = 0;

  virtual bool modifyHostname(std::string newHostname) = 0;

  virtual bool removeAllStorage() = 0;

  virtual ~ConfigStorage() {}
};

class FileStorage : public ConfigStorage {
  std::string configDir;

 public:
  explicit FileStorage(std::string configDir) : configDir(configDir) {}

  void writeWhitelist(Whitelist whitelist) override {
    std::ofstream f(configDir + "whitelist.tmp");
    if (!f.good()) {
      LOG("failed to write whitelist file (%s)",
          (configDir + "whitelist.tmp").c_str());
      return;
    }

    if (whitelist.isEnabled)
      f << "enabled\n";
    else
      f << "disabled\n";

    for (std::string dev : whitelist.devices)
      f << IpAddress::fromBinary(dev).str() << "\n";

    f.close();

    if (renameFile((configDir + "whitelist.tmp").c_str(),
                   (configDir + "whitelist").c_str()) != 0) {
      LOG("failed to write whitelist file");
    }
  }

  Whitelist readWhitelist() override {
    Whitelist result;

    std::ifstream f = openFile(configDir + "whitelist");

    if (!f.good()) {
      LOG("could not open whitelist file");
      result.isEnabled = true;
      return result;
    }

    std::string enabledStr;
    f >> enabledStr;
    result.isEnabled = (enabledStr != "disabled");
    LOG("whitelist is %s", result.isEnabled ? "enabled" : "disabled");

    std::string id;
    while (f >> id) {
      LOG("%s is whitelisted", id.c_str());
      id = IpAddress::parse(id.c_str()).toBinary();
      result.devices.insert(id);
    }
    return result;
  }

  WebsetupConfig readWebsetupConfig() override {
    std::string sharedSecret;
    std::vector<IpAddress> managerWhitelist;

    std::ifstream f(configDir + "websetup");

    if (f.good()) {
      f >> sharedSecret;
      std::string ip;
      while (f >> ip) {
        managerWhitelist.push_back(IpAddress::parse(ip.c_str()));
      }
    } else {
      LOG("could not read websetup config");
    }

    return {sharedSecret, managerWhitelist};
  }

  bool writeWebsetupConfig(WebsetupConfig config) override {
    {
      std::ofstream f(configDir + "websetup.tmp");

      f << config.websetupSecret << "\n";
      for (IpAddress addr : config.managerWhitelist)
        f << addr.str() << "\n";
    }

    if (renameFile((configDir + "websetup.tmp").c_str(),
                   (configDir + "websetup").c_str()) != 0) {
      LOG("failed to write websetup file");
      return false;
    }

    return true;
  }

  bool modifyHostname(std::string newHostname) override {
    return ServiceHelper::modifyHostname(newHostname);
  }

  bool removeAllStorage() override {
    if (unlink((configDir + "websetup").c_str()) != 0 && errno != ENOENT)
      return false;
    if (unlink((configDir + "whitelist").c_str()) != 0 && errno != ENOENT)
      return false;
    return true;
  }
};

ConfigStorage* createStorage(std::string configDir) {
  return new FileStorage(configDir);
}

void migrateToConfigTable(ConfigTable* configTable, std::string configDir) {
  if (!std::ifstream(configDir + "whitelist").good() &&
      !std::ifstream(configDir + "websetup").good())
    return;

  LOG("migrating old configuration format to SQlite database...");

  ConfigStorage* storage = createStorage(configDir);

  Whitelist whitelist = storage->readWhitelist();
  WebsetupConfig websetupConfig = storage->readWebsetupConfig();

  auto hostsData = ServiceHelper::getHostsFileAtStartup();

  configTable->runInTransaction([&]() {
    if (!whitelist.isEnabled)
      configTable->insert(
          ConfigRow{"manual", "config", "whitelist-enabled", "false"});
    for (DeviceId host : whitelist.devices) {
      configTable->insert(ConfigRow{"manual", "whitelist",
                                    IpAddress::fromBinary(host).str(), "1"});
    }

    for (auto p : hostsData) {
      configTable->insert(ConfigRow{"manual", "host", p.second, p.first.str()});
    }

    if (websetupConfig.websetupSecret.size() > 0)
      configTable->insert(ConfigRow{"manual", "config", "websetup-secret",
                                    websetupConfig.websetupSecret});

    for (IpAddress host : websetupConfig.managerWhitelist) {
      configTable->insert(ConfigRow{"manual", "manager", host.str(), "1"});
    }
  });

  if (!storage->removeAllStorage()) {
    LOG("failed to remove old files!");
  }

  LOG("migration finished");
}

}  // namespace LegacyFileStorage
