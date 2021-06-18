#include "base_config.h"
#include "husarnet_config.h"
#include "husarnet_crypto.h"
#include "main_common.h"
#include "port.h"
#include "self_hosted.h"
#include "service.h"
#include "service_helper.h"
#include "smartcard_client.h"
#include "sockets.h"
#include "util.h"
#include "license_management.h"
#include <fstream>
#include <iostream>
#include <sys/un.h>
#include <unistd.h>

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

  husarnet whitelist ls
    Lists all addresses added to whitelist.

  husarnet status
    Displays current connectivity status.

  husarnet genid
    Generate identity.

  husarnet websetup
    Run the node configuration utility in web browser.

  husarnet daemon
    Run the Husarnet daemon (typically started by systemd).
    
  husarnet setup-server [address]
    Connect to other Husarnet Dashboards and Husarnet Base Servers (remember to restart Husarnet to enable new setting).

  husarnet join [join code] [hostname]
    Connect to Husarnet network with given join code as with specified hostname.

  husarnet host add [hostname] [address]
    Assign alternative hostanme to Husarnet address.

  husarnet host rm [hostname] [address]
    Remove alternative hostname.

  husarnet version
    Displays version currently in use.

  husarnet status-json
    Retruns the same information that husarnet status returns but in json format.  
    )" << std::endl;
    exit(1);
}

std::string sendMsg(std::string msg) {
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);

    sockaddr_un servaddr = {0};
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sun_family = AF_UNIX;
    strncpy(servaddr.sun_path, (configDir + "control.sock").c_str(), sizeof(servaddr.sun_path) - 1);

    if (connect(fd, (sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
        if (getuid() == 0) {
            LOG ("failed to connect to Husarnet control socket, is the daemon running?");
        } else {
            LOG ("failed to connect to Husarnet control socket, please run as root");
        }
        exit(1);
    }

    send(fd, msg.data(), msg.size(), 0);
    std::string resp;
    resp.resize(128 * 1024);
    long ret = ::recv(fd, &resp[0], resp.size(), 0);

    if (ret < 0) {
        LOG("didn't receive response");
        return "";
    }

    return resp.substr(0, ret);
}

void sendMsgChecked(std::string msg) {
    std::string result = sendMsg(msg);
    if (result != "ok") {
        LOG("Husarnet daemon returned error (%s). Check log.", result.c_str());
        exit(1);
    }
}

std::string getMyId() {
    std::ifstream f (configDir + "/id");
    if (!f.good()) {
        LOG("Failed to access configuration file.");
        if (getuid() != 0) LOG("Please run as root.");
        exit(1);
    }
    std::string id;
    f >> id;
    return IpAddress::parse(id.c_str()).toBinary();
}

std::string get_manage_url(const std::string &dashboardUrl, std::string webtoken) {
    return dashboardUrl + "/husarnet/" + webtoken;
}

void print_websetup_message(const std::string &dashboardUrl, std::string webtoken) {
    std::cerr << "Go to " + get_manage_url(dashboardUrl, webtoken) + " to manage your network from web browser." << std::endl;
}

void l2setup() {
    if (system("ip link del hnetl2 2>/dev/null") != 0) {}

    std::ifstream f ("/var/lib/husarnet/id");
    if (!f.good()) { LOG("failed to open identity file"); exit(1); }
    std::string ip;
    f >> ip;

    std::string mac = IpAddress::parse(ip).toBinary().substr(2, 6);
    mac[0] = (((uint8_t)mac[0]) | 0x02) & 0xFE;
    char macStr[100];
    sprintf(macStr, "%02x:%02x:%02x:%02x:%02x:%02x", uint8_t(mac[0]), uint8_t(mac[1]), uint8_t(mac[2]), uint8_t(mac[3]), uint8_t(mac[4]), uint8_t(mac[5]));

    std::string cmd = "ip link add hnetl2 address ";
    cmd += macStr;
    cmd += " type vxlan id 94 group ff15:f2d3:a389::2 dev hnet0 dstport 4789";
    if (system(cmd.c_str()) != 0) exit(1);

    if (system("ip link set dev hnetl2 up") != 0) exit(1);
}

BaseConfig* loadBaseConfig() {
  auto path = configDir + "/license.json";
  std::ifstream input(path);
  if (input.is_open()) {
    std::string str((std::istreambuf_iterator<char>(input)),
                    std::istreambuf_iterator<char>());
    return new BaseConfig(str);
  } else {
    LOG("Can't open license.json, reset required.");
    exit(1);
  }
}

std::string parseVersion(std::string info){

  std::size_t pos1 = info.find("Version:");
  std::size_t pos2 = info.find("\n");
  std::string version = info.substr(pos1+8,pos2-pos1-8); 
  return version;
}

void removeLicense() {
    std::string configDir = prepareConfigDir();
    std::string licensePath = configDir + "license.json";

    unlink(licensePath.c_str());
}

void refreshLicense() {
    auto baseConfig = loadBaseConfig();
    if (!baseConfig->isDefault()) {
        std::string hostname = baseConfig->getDashboardUrl();
        if (hostname.substr(0, 8) == "https://") {
            hostname = hostname.substr(8);
        } else if (hostname.substr(0, 7) == "http://") {
            hostname = hostname.substr(7);
        } else {
            abort();
        }

        std::size_t slashLocation = hostname.find('/');
        if (slashLocation != std::string::npos) {
            hostname = hostname.substr(0, slashLocation);
        }

        IpAddress ip = OsSocket::resolveToIp(hostname);
        InetAddress address{ip, 80};
        retrieveLicense(address);
    }
}

int main(int argc, const char** args) {
    initPort();
    if (argc < 2) {
        printUsage();
    }

    configDir = (getenv("HUSARNET_CONFIG") ? getenv("HUSARNET_CONFIG") : "/var/lib/husarnet/");

    std::string cmd = args[1];
    if (cmd == "daemon") {
        serviceMain();
    } else if (cmd == "fork-daemon") {
        serviceMain(true);
    } else if (cmd == "genid") {
        auto p = NgSocketCrypto::generateId();

        std::cout << IpAddress::fromBinary(NgSocketCrypto::pubkeyToDeviceId(p.first)).str()
                  << " " << encodeHex(p.first) << " " << encodeHex(p.second) << std::endl;
    } else if (cmd == "websetup") {
        auto baseConfig = loadBaseConfig();
        if (argc > 2 && std::string(args[2]) == "--link-only") {
            std::cout << get_manage_url(baseConfig->getDashboardUrl(), getWebsetupData()) << std::endl;
        } else {
            print_websetup_message(baseConfig->getDashboardUrl(), getWebsetupData());
        }
    } else if (cmd == "join" && (argc == 3 || argc == 4)) {
        getWebsetupData();

        sendMsg("reset-received-init-response\n");

        while (sendMsg("has-received-init-response\n") != "yes") {
            LOG("joining...");
            std::string code = args[2];
            std::string hostname = (argc == 4) ? args[3] : "";
            sendMsg("join\n" + code + "\n" + hostname);

            sleep(2);
        }
    } else if ((cmd == "host" || cmd == "hosts") && argc >= 3) {
        std::string subcmd = args[2];
        if ((subcmd == "add" || subcmd == "rm") && argc == 5) {
            std::string name = args[3];
            IpAddress ip = IpAddress::parse(args[4]);
            if (!ip) {
                LOG ("invalid IP address");
                printUsage();
            }
            sendMsgChecked("host-" + subcmd + "\n" + ip.str() + " " + name);
            sendMsgChecked("whitelist-" + subcmd + "\n" + encodeHex(ip.toBinary()));
        } else {
            printUsage();
        }
    } else if (cmd == "whitelist" && argc >= 3) {
        std::string subcmd = args[2];
        if ((subcmd == "add" || subcmd == "rm") && argc >= 4) {
            for (int i=3; i < argc; i++) {
                IpAddress ip = IpAddress::parse(args[i]);
                if (!ip) {
                    LOG ("invalid IP address");
                    printUsage();
                }
                sendMsgChecked("whitelist-" + subcmd + "\n" + encodeHex(ip.toBinary()));
            }
        } else if (subcmd == "enable" && argc == 3) {
            sendMsgChecked("whitelist-enable\n");
        } else if (subcmd == "disable" && argc==4) {
            std::string flag = args[3];
            if (flag == "--noinput") {
                sendMsgChecked("whitelist-disable\n");
            } else {
                std::cout << "Disabling whitelist may pose some risks, make sure You know what You do.\n If You wish to disable whitelist run this with --noinput option"; 
            }
        }else if (subcmd == "disable" && argc == 3) {
            std::cout << "Disabling whitelist may pose some risks, make sure You know what You do.\n If You wish to disable whitelist run this with --noinput option";
        } else if (subcmd == "ls" && argc == 3) {
            std::cout << sendMsg("whitelist-ls\n") << std::endl;
        }else {
            printUsage();
        }
    } else if (cmd == "init-websetup" && argc == 3) {
        sendMsgChecked("set-websetup-token\n" + std::string(args[2]));
    } else if (cmd == "status") {
        std::cout << sendMsg("status\n") << std::endl;
    } else if (cmd == "status-json") {
        std::cout << sendMsg("status-json\n") << std::endl;
    } else if (cmd == "l2-setup") {
        l2setup();
    } else if (cmd == "version") {
         std::cout << parseVersion(sendMsg("status\n")) << std::endl;
    } else if (cmd == "setup-server" && argc == 3) {
        if (std::string(args[2]) == "default") {
            removeLicense();
        } else {
            IpAddress ip = OsSocket::resolveToIp(args[2]);
            InetAddress address{ip, 80};
            retrieveLicense(address);
        }
    } else if (cmd == "refresh-license") {
        refreshLicense();
    } else {
        printUsage();
    }
}
