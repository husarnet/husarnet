#pragma once
#include <string>
#include "husarnet_crypto.h"

namespace FileStorage {

std::string getConfigDir();
void prepareConfigDir();

std::ifstream openFile(std::string path);
std::ifstream writeFile(std::string path, std::string content);

Identity* readIdentity();
void generateAndWriteId();

void generateAndWriteHttpSecret();
std::string readHttpSecret();

void saveIp6tablesRuleForDeletion(std::string rule);

inline const std::string idFilePath() {
  return getConfigDir() + "id";
}
inline const std::string httpSecretFilePath() {
  return getConfigDir() + "http_secret";
}
inline const std::string settingsFilePath() {
  return getConfigDir() + "settings.json";
}
inline const std::string licenseFilePath() {
  return getConfigDir() + "license.json";
}
inline const std::string controlSocketPath() {
  return getConfigDir() + "control.sock";
}
inline const std::string ip6tablesRulesLogPath() {
  return getConfigDir() + "ip6tables_rules";
}

}  // namespace FileStorage
