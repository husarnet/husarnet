#include <unistd.h>
#include <fstream>
#include <iostream>
#include "base_config.h"
#include "husarnet_config.h"
#include "husarnet_crypto.h"
#include "main_common.h"
#include "port.h"
#include "service_helper.h"
#include "sockets.h"
#include "util.h"

void serviceMain();
std::string getConfigDir();

static std::string configDir;

void printUsage() {
  std::cerr << R"(Usage:

  husarnet whitelist add [address]
    Add fc94 IP address to whitelist.

  husarnet whitelist rm [address]
    Remove fc94 IP address from whitelist.

  husarnet whitelist enable
    Enable address whitelist (default).

  husarnet whitelist disable
    Disable address whitelist (allow everyone, make sure to configure local firewall).

  husarnet status
    Displays current connectivity status.

  husarnet websetup
    Run the node configuration utility in web browser.

  husarnet daemon
    Run the Husarnet daemon (typically started as a service).)"
            << std::endl;
  exit(1);
}

std::string getControlSharedSecret(std::string configDir);

std::string sendMsg(std::string msg) {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  sockaddr_in6 servaddr =
      OsSocket::sockaddrFromIp(InetAddress::parse("127.0.0.1:5579"));

  if (connect(fd, (sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
    LOG("failed to connect to Husarnet control socket (%d)", WSAGetLastError());
    exit(1);
  }

  msg = getControlSharedSecret(configDir) + msg;

  send(fd, msg.data(), msg.size(), 0);
  shutdown(fd, SD_SEND);
  std::string resp;
  resp.resize(128 * 1024);
  long ret = ::recv(fd, &resp[0], resp.size(), 0);

  if (ret < 0) {
    LOG("didn't receive response");
    return "";
  }

  return resp.substr(0, ret);
}

std::string getMyId() {
  std::ifstream f(configDir + "/id");
  if (!f.good()) {
    LOG("Failed to access configuration file.");
    LOG("Please make sure you run the utility as an administrator.");
    exit(1);
  }
  std::string id;
  f >> id;
  return IpAddress::parse(id.c_str()).toBinary();
}

std::string get_manage_url(const std::string& dashboardUrl,
                           std::string webtoken) {
  return dashboardUrl + "/husarnet/" + webtoken;
}

void print_websetup_message(const std::string& dashboardUrl,
                            std::string webtoken) {
  std::cerr << "Go to " + get_manage_url(dashboardUrl, webtoken) +
                   " to manage your network from web browser."
            << std::endl;
}

int main(int argc, const char** args) {
  initPort();

  if (argc < 2) {
    printUsage();
  }

  // TODO load license.json if exists
  BaseConfig* baseConfig = new BaseConfig;

  configDir = getConfigDir();

  std::string cmd = args[1];
  if (cmd == "daemon") {
    serviceMain();
  } else if (cmd == "genid") {
    auto p = NgSocketCrypto::generateId();

    std::cout << IpAddress::fromBinary(
                     NgSocketCrypto::pubkeyToDeviceId(p.first))
                     .str()
              << " " << encodeHex(p.first) << " " << encodeHex(p.second)
              << std::endl;
  } else if (cmd == "websetup") {
    if (argc > 2 && std::string(args[2]) == "--link-only") {
      std::cout << get_manage_url(baseConfig->getDashboardUrl(),
                                  getWebsetupData())
                << std::endl;
    } else {
      print_websetup_message(baseConfig->getDashboardUrl(), getWebsetupData());
    }
  } else if (cmd == "join" && (argc == 3 || argc == 4)) {
    getWebsetupData();

    sendMsg("reset-received-init-response\n");

    for (int i = 0; i < 5; i++) {
      if (sendMsg("has-received-init-response\n") == "yes") {
        printf("Network joined successfully.\n");
        goto success;
      }

      std::string code = args[2];
      std::string hostname = (argc == 4) ? args[3] : "";
      sendMsg("join\n" + code + "\n" + hostname);

      sleep(2);
    }
    printf(
        "Could not join the network. Check if code is valid or try again "
        "later.\n");
  success:
    (void)0;
  } else if ((cmd == "host" || cmd == "hosts") && argc >= 3) {
    std::string subcmd = args[2];
    if ((subcmd == "add" || subcmd == "rm") && argc == 5) {
      std::string name = args[3];
      IpAddress ip = IpAddress::parse(args[4]);
      if (!ip) {
        LOG("invalid IP address");
        printUsage();
      }
      sendMsg("host-" + subcmd + "\n" + ip.str() + " " + name);
      sendMsg("whitelist-" + subcmd + "\n" + encodeHex(ip.toBinary()));
    } else {
      printUsage();
    }
  } else if (cmd == "whitelist" && argc >= 3) {
    std::string subcmd = args[2];
    if ((subcmd == "add" || subcmd == "rm") && argc >= 4) {
      for (int i = 3; i < argc; i++) {
        IpAddress ip = IpAddress::parse(args[i]);
        if (!ip) {
          LOG("invalid IP address");
          printUsage();
        }
        sendMsg("whitelist-" + subcmd + "\n" + encodeHex(ip.toBinary()));
      }
    } else if (subcmd == "enable" && argc == 3) {
      sendMsg("whitelist-enable\n");
    } else if (subcmd == "disable" && argc == 3) {
      sendMsg("whitelist-disable\n");
    } else {
      printUsage();
    }
  } else if (cmd == "status") {
    std::cout << sendMsg("status\n") << std::endl;
  } else if (cmd == "status-json") {
    std::cout << sendMsg("status-json\n") << std::endl;
  } else {
    printUsage();
  }
}
