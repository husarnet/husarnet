// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// #include "filestorage.h"
// #include <time.h>
// #include <fstream>
// #include "husarnet_crypto.h"
// #include "port.h"
// #include "service_helper.h"
// #include "util.h"

// // TODO move file storage abstract to privileged section
// namespace FileStorage {

// std::string getConfigDir() {
//   return getenv("HUSARNET_CONFIG" ? getenv("HUSARNET_CONFIG")
//                                   : "/var/lib/husarnet/");
// }

// void prepareConfigDir() {
//   auto configDir = getConfigDir();

//   // TODO fix this
//   // mkdir(configDir.c_str(), 0700);
//   // chmod(configDir.c_str(), 0700);
// }

// void generateAndWriteHttpSecret() {
//   LOG("Generatting http secret...");
//   std::string secret = generateRandomString(64);
//   std::ofstream f(httpSecretFilePath());
//   if (!f.good()) {
//     LOG("failed to write: %s", httpSecretFilePath().c_str());
//     exit(1);
//   }
//   f << secret << std::endl;
// }

// std::string readHttpSecret() {
//   std::ifstream f = openFile(httpSecretFilePath());
//   if (!f.good())
//     exit(1);
//   std::string secret;
//   f >> secret;
//   return secret;
// }

// void saveIp6tablesRuleForDeletion(std::string rule) {
//   LOG("Saving Ip6tables rules for later deletion...");
//   std::ofstream f(ip6tablesRulesLogPath(), std::ios_base::app);
//   if (!f.good()) {
//     LOG("failed to write: %s", ip6tablesRulesLogPath().c_str());
//     exit(1);
//   }
//   f << rule << std::endl;
// }

// }  // namespace FileStorage
