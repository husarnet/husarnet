// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// #include <sys/un.h>
// #include <unistd.h>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include "base_config.h"
// #include "filestorage.h"
// #include "http_handling.h"
// #include "httplib.h"
// #include "husarnet_config.h"
// #include "husarnet_crypto.h"
// #include "licensing.h"
// #include "licensing_unix.h"
// #include "main_common.h"
// #include "port.h"
// #include "service.h"
// #include "service_helper.h"
// #include "smartcard_client.h"
// #include "sockets.h"
// #include "util.h"

// static std::string configDir;

// std::string getMyId() {
//   std::ifstream f(FileStorage::idFilePath(configDir));
//   if (!f.good()) {
//     LOG("Failed to access configuration file.");
//     if (getuid() != 0)
//       LOG("Please run as root.");
//     exit(1);
//   }
//   std::string id;
//   f >> id;
//   return IpAddress::parse(id.c_str()).toBinary();
// }

// std::string get_manage_url(const std::string& dashboardUrl,
//                            std::string webtoken) {
//   return dashboardUrl + "/husarnet/" + webtoken;
// }

// void print_websetup_message(const std::string& dashboardUrl,
//                             std::string webtoken) {
//   std::cerr << "Go to " + get_manage_url(dashboardUrl, webtoken) +
//                    " to manage your network from web browser."
//             << std::endl;
// }

// void l2setup() {
//   if (system("ip link del hnetl2 2>/dev/null") != 0) {
//   }

//   std::ifstream f(FileStorage::idFilePath(configDir));
//   if (!f.good()) {
//     LOG("failed to open identity file");
//     exit(1);
//   }
//   std::string ip;
//   f >> ip;

//   std::string mac = IpAddress::parse(ip).toBinary().substr(2, 6);
//   mac[0] = (((uint8_t)mac[0]) | 0x02) & 0xFE;
//   char macStr[100];
//   sprintf(macStr, "%02x:%02x:%02x:%02x:%02x:%02x", uint8_t(mac[0]),
//           uint8_t(mac[1]), uint8_t(mac[2]), uint8_t(mac[3]), uint8_t(mac[4]),
//           uint8_t(mac[5]));

//   std::string cmd = "ip link add hnetl2 address ";
//   cmd += macStr;
//   cmd += " type vxlan id 94 group ff15:f2d3:a389::2 dev hnet0 dstport 4789";
//   if (system(cmd.c_str()) != 0)
//     exit(1);

//   if (system("ip link set dev hnetl2 up") != 0)
//     exit(1);
// }

// BaseConfig* loadBaseConfig() {
//   auto path = FileStorage::licenseFilePath(configDir);
//   std::ifstream input(path);
//   if (input.is_open()) {
//     std::string str((std::istreambuf_iterator<char>(input)),
//                     std::istreambuf_iterator<char>());
//     return new BaseConfig(str);
//   } else {
//     LOG("Can't open license.json, reset required.");
//     exit(1);
//   }
// }

// std::string parseVersion(std::string info) {
//   std::size_t pos1 = info.find("Version:");
//   std::size_t pos2 = info.find("\n");
//   std::string version = info.substr(pos1 + 8, pos2 - pos1 - 8);
//   return version;
// }
