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

// std::string sendMsg(std::string msg) {
//   int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);

//   sockaddr_un servaddr = {0};
//   memset(&servaddr, 0, sizeof(servaddr));
//   servaddr.sun_family = AF_UNIX;
//   strncpy(servaddr.sun_path,
//           (FileStorage::controlSocketPath(configDir)).c_str(),
//           sizeof(servaddr.sun_path) - 1);

//   if (connect(fd, (sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
//     if (getuid() == 0) {
//       LOG("failed to connect to Husarnet control socket, is the daemon "
//           "running?");
//     } else {
//       LOG("failed to connect to Husarnet control socket, please run as
//       root");
//     }
//     exit(1);
//   }

//   send(fd, msg.data(), msg.size(), 0);
//   std::string resp;
//   resp.resize(128 * 1024);
//   long ret = ::recv(fd, &resp[0], resp.size(), 0);

//   if (ret < 0) {
//     LOG("didn't receive response");
//     return "";
//   }

//   return resp.substr(0, ret);
// }

// void sendMsgChecked(std::string msg) {
//   std::string result = sendMsg(msg);
//   if (result != "ok") {
//     LOG("Husarnet daemon returned error (%s). Check log.", result.c_str());
//     exit(1);
//   }
// }

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

// int main_old(int argc, const char** args) {
//   configDir = (getenv("HUSARNET_CONFIG") ? getenv("HUSARNET_CONFIG")
//                                          : "/var/lib/husarnet/");

//   httplib::Client cli("http://127.0.0.1:9999");

//   std::string cmd = args[1];
//   if (cmd == "websetup") {
//     auto baseConfig = loadBaseConfig();
//     if (argc > 2 && std::string(args[2]) == "--link-only") {
//       std::cout << get_manage_url(baseConfig->getDashboardUrl(),
//                                   getWebsetupData())
//                 << std::endl;
//     } else {
//       print_websetup_message(baseConfig->getDashboardUrl(),
//       getWebsetupData());
//     }
//   } else if (cmd == "join" && (argc == 3 || argc == 4)) {
//     getWebsetupData();
//     auto params = get_http_params_with_secret(configDir);
//     auto res = cli.Post("/control/reset-received-init-response", params);
//     if (is_invalid_response(res)) {
//       handle_invalid_response(res);
//       return 1;
//     }
//     sleep(2);
//     int attemptsBeforeTimeout = 7;
//     bool timedOut = false;
//     while ((res = cli.Post("/control/has-received-init-response", params)) &&
//            (!is_invalid_response(res)) && res->body != "yes") {
//       std::string code = args[2];
//       std::string hostname = (argc == 4) ? args[3] : "";
//       auto params = get_http_params_with_secret(configDir);
//       params.emplace("code", code);
//       params.emplace("hostname", hostname);
//       auto res = cli.Post("/control/join", params);
//       if (is_invalid_response(res)) {
//         handle_invalid_response(res);
//         return 1;
//       }
//       if (attemptsBeforeTimeout == 0) {
//         timedOut = true;
//         LOG("Request timed out.\nCommon reasons:\n1) This device is already "
//             "added by Join Code to another network (maybe by another
//             user)\n2) " "You have exceeded maximum number of devices for your
//             plan - " "upgrade your account at "
//             "https://app.husarnet.com/billing/account/\n3) If you use "
//             "self-hosted Husarnet infrastructure make sure your Husarnet "
//             "Client is configured to use it (eg. sudo husarnet setup-server "
//             "app.husarnet.mydomain.com)\nIf the problem persists, find more "
//             "information here: "
//             "https://husarnet.com/docs/tutorial-troubleshooting/");
//         break;
//       }
//       attemptsBeforeTimeout--;
//       LOG("joining...");
//       sleep(2);
//     }

//     if (!timedOut) {
//       LOG("done.");
//     }
//   } else if ((cmd == "host" || cmd == "hosts") && argc >= 3) {
//     std::string subcmd = args[2];
//     if ((subcmd == "add" || subcmd == "rm") && argc == 5) {
//       std::string name = args[3];
//       IpAddress ip = IpAddress::parse(args[4]);
//       if (!ip) {
//         LOG("invalid IP address");
//         printUsage();
//       }
//       httplib::Params params1 = get_http_params_with_secret(configDir);
//       params1.emplace("hostname", name);
//       params1.emplace("address", ip.str());
//       auto res1 = cli.Post(("/control/host/" + subcmd).c_str(), params1);
//       if (is_invalid_response(res1)) {
//         handle_invalid_response(res1);
//         return 1;
//       }
//       if (res1->body != "ok") {
//         LOG("Husarnet daemon returned error (%s). Check log.",
//             res1->body.c_str());
//         exit(1);
//       }
//       httplib::Params params2 = get_http_params_with_secret(configDir);
//       params2.emplace("address", encodeHex(ip.toBinary()));
//       auto res2 = cli.Post(("/control/whitelist/" + subcmd).c_str(),
//       params2); if (is_invalid_response(res2)) {
//         handle_invalid_response(res2);
//         return 1;
//       }
//       if (res2->body != "ok") {
//         LOG("Husarnet daemon returned error (%s). Check log.",
//             res2->body.c_str());
//         exit(1);
//       }
//     } else {
//       printUsage();
//     }
//   } else if (cmd == "whitelist" && argc >= 3) {
//     std::string subcmd = args[2];
//     if ((subcmd == "add" || subcmd == "rm") && argc >= 4) {
//       for (int i = 3; i < argc; i++) {
//         IpAddress ip = IpAddress::parse(args[i]);
//         if (!ip) {
//           LOG("invalid IP address");
//           printUsage();
//         }
//         httplib::Params params = get_http_params_with_secret(configDir);
//         params.emplace("address", encodeHex(ip.toBinary()));
//         auto res = cli.Post(("/control/whitelist/" + subcmd).c_str(),
//         params); if (is_invalid_response(res)) {
//           handle_invalid_response(res);
//           return 1;
//         }
//         if (res->body != "ok") {
//           LOG("Husarnet daemon returned error (%s). Check log.",
//               res->body.c_str());
//           exit(1);
//         }
//       }
//     } else if (subcmd == "enable" && argc == 3) {
//       auto params = get_http_params_with_secret(configDir);
//       auto res = cli.Post("/control/whitelist/enable", params);
//       if (is_invalid_response(res)) {
//         handle_invalid_response(res);
//         return 1;
//       }
//       if (res->body != "ok") {
//         LOG("Husarnet daemon returned error (%s). Check log.",
//             res->body.c_str());
//         exit(1);
//       }
//     } else if (subcmd == "disable" && argc == 4) {
//       std::string flag = args[3];
//       if (flag == "--noinput") {
//         auto params = get_http_params_with_secret(configDir);
//         auto res = cli.Post("/control/whitelist/disable", params);
//         if (is_invalid_response(res)) {
//           handle_invalid_response(res);
//           return 1;
//         }
//         if (res->body != "ok") {
//           LOG("Husarnet daemon returned error (%s). Check log.",
//               res->body.c_str());
//           exit(1);
//         }
//       } else {
//         std::cout << "Disabling whitelist may pose some risks, make sure You
//         "
//                      "know what You do.\n If You wish to disable whitelist
//                      run " "this with --noinput option";
//       }
//     } else if (subcmd == "disable" && argc == 3) {
//       std::cout << "Disabling whitelist may pose some risks, make sure You "
//                    "know what You do.\n If You wish to disable whitelist run
//                    " "this with --noinput option";
//     } else if (subcmd == "ls" && argc == 3) {
//       auto res = cli.Get("/control/whitelist/ls");
//       if (is_invalid_response(res)) {
//         handle_invalid_response(res);
//         return 1;
//       }
//       std::cout << res->body << std::endl;
//     } else {
//       printUsage();
//     }
//   } else if (cmd == "status") {
//     auto res = cli.Get("/control/status");
//     if (is_invalid_response(res)) {
//       handle_invalid_response(res);
//       return 1;
//     }
//     std::cout << res->body << std::endl;
//   } else if (cmd == "status-json") {
//     auto res = cli.Get("/control/status-json");
//     if (is_invalid_response(res)) {
//       handle_invalid_response(res);
//       return 1;
//     }
//     std::cout << res->body << std::endl;
//   } else if (cmd == "l2-setup") {
//     l2setup();
//   } else if (cmd == "version") {
//     auto res = cli.Get("/control/status");
//     if (is_invalid_response(res)) {
//       handle_invalid_response(res);
//       return 1;
//     }
//     std::cout << parseVersion(res->body) << std::endl;
//   } else if (cmd == "verbosity" && argc >= 3) {
//     std::string tmp = args[2];
//     std::istringstream reader(tmp);
//     uint verbosity;
//     reader >> verbosity;
//     auto params = get_http_params_with_secret(configDir);
//     params.emplace("verbosity", std::to_string(verbosity));
//     auto res = cli.Post("/control/logs/verbosity", params);
//     if (is_invalid_response(res)) {
//       handle_invalid_response(res);
//       return 1;
//     }
//     if (res->body != "ok") {
//       LOG("Husarnet daemon returned error (%s). Check log.",
//       res->body.c_str()); exit(1);
//     }
//   } else if (cmd == "verbosity") {
//     auto res = cli.Get("/control/logs-verbosity");
//     if (is_invalid_response(res)) {
//       handle_invalid_response(res);
//       return 1;
//     }
//     std::cout << res->body << std::endl;
//   } else if (cmd == "logs" && argc == 4) {
//     std::string subcmd = args[2];
//     std::string size = args[3];
//     if (subcmd == "size") {
//       auto params = get_http_params_with_secret(configDir);
//       params.emplace("size", size);
//       auto res = cli.Post("/control/logs/size", params);
//       if (is_invalid_response(res)) {
//         handle_invalid_response(res);
//         return 1;
//       }
//       if (res->body != "ok") {
//         LOG("Husarnet daemon returned error (%s). Check log.",
//             res->body.c_str());
//         exit(1);
//       }
//     } else {
//       printUsage();
//     }
//   } else if (cmd == "logs" && argc == 3) {
//     std::string subcmd = args[2];
//     if (subcmd == "size") {
//       auto res = cli.Get("/control/logs/size");
//       if (is_invalid_response(res)) {
//         handle_invalid_response(res);
//         return 1;
//       }
//       std::cout << res->body << std::endl;
//     } else if (subcmd == "current") {
//       auto res = cli.Get("/control/logs/currentSize");
//       if (is_invalid_response(res)) {
//         handle_invalid_response(res);
//         return 1;
//       }
//       std::cout << res->body << std::endl;
//     } else {
//       printUsage();
//     }
//   } else if (cmd == "logs" && argc < 3) {
//     auto res = cli.Get("/control/logs");
//     if (is_invalid_response(res)) {
//       handle_invalid_response(res);
//       return 1;
//     }
//     std::cout << res->body << std::endl;
//   } else if (cmd == "setup-server" && argc == 3) {
//     if (std::string(args[2]) == "default") {
//       removeLicense();
//     } else {
//       IpAddress ip = OsSocket::resolveToIp(args[2]);
//       InetAddress address{ip, 80};
//       retrieveLicense(address);
//     }
//   } else if (cmd == "refresh-license") {
//     refreshLicense();
//   } else if (cmd == "wait") {
//     const int limit = 300;

//     std::cout << "Will wait for the deamon to become responsive for up to "
//               << limit << " seconds." << std::endl;

//     int i = 0;
//     for (; i < limit; i++) {
//       auto res = cli.Get("/ping");
//       if (is_invalid_response(res)) {
//         std::cout << "." << std::flush;
//         sleep(1);
//         continue;
//       } else {
//         std::cout << "connection established!" << std::endl;
//         break;
//       }
//     }
//     if (i >= limit) {
//       std::cout << std::endl << "Unable to establish connection." <<
//       std::endl; return 1;
//     }
//   } else {
//     printUsage();
//   }

//   return 0;
// }