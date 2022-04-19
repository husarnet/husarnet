// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include "ngsocket_crypto.h"

// TODO wywalic caly ten plik - powinien byc proxowany przez privileged

namespace FileStorage {

  std::string getConfigDir();
  void prepareConfigDir();

  std::ifstream openFile(std::string path);
  std::ifstream writeFile(std::string path, std::string content);

  Identity* readIdentity();
  void generateAndWriteId();

  void generateAndWriteHttpSecret();
  // std::string readHttpSecret();

  void saveIp6tablesRuleForDeletion(std::string rule);

  inline const std::string idFilePath() { return getConfigDir() + "id"; }
  inline const std::string httpSecretFilePath()
  {
    return getConfigDir() + "http_secret";
  }
  inline const std::string settingsFilePath()
  {
    return getConfigDir() + "settings.json";
  }
  inline const std::string licenseFilePath()
  {
    return getConfigDir() + "license.json";
  }
  inline const std::string controlSocketPath()
  {
    return getConfigDir() + "control.sock";
  }
  inline const std::string ip6tablesRulesLogPath()
  {
    return getConfigDir() + "ip6tables_rules";
  }

}  // namespace FileStorage
