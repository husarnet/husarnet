#include <signal.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include "base_config.h"
#include "configmanager.h"
#include "dns_server.h"
#include "filestorage.h"
#include "husarnet.h"
#include "husarnet_config.h"
#include "husarnet_crypto.h"
#include "husarnet_secure.h"
#include "l2unwrapper.h"
#include "legacy_filestorage.h"
#include "licensing_unix.h"
#include "logmanager.h"
#include "network_dev.h"
#include "port.h"
#include "service_helper.h"
#include "settings.h"
#include "smartcard_client.h"
#include "sockets.h"
#include "sqlite_configtable.h"
#include "tun.h"
#include "util.h"

#include <linux/capability.h>
#include <linux/securebits.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

using namespace std::placeholders;

void bindControlSocket(ConfigManager& manager, std::string path) {
  // TODO: move control socket support to separate threads
  sockaddr_un servaddr = {0};
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sun_family = AF_UNIX;
  strncpy(servaddr.sun_path, path.c_str(), sizeof(servaddr.sun_path) - 1);

  unlink(path.c_str());

  int sockfd =
      socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (bind(sockfd, (sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
    LOG("failed to bind control socket");
    abort();
  }
  if (listen(sockfd, SOMAXCONN) != 0)
    abort();

  OsSocket::bindCustomFd(sockfd, [sockfd, &manager]() {
    int fd = accept(sockfd, nullptr, nullptr);
    if (fd < 0) {
      LOG("control socket accept failed");
      return;
    }

    std::string buf;
    buf.resize(1024);
    long ret = ::recv(fd, &buf[0], buf.size(), 0);
    if (ret < 0) {
      LOG("failed to receive control packet");
      return;
    }
    std::string response = manager.handleControlPacket(buf.substr(0, ret));
    if (response.size() > 0) {
      long ret = ::send(fd, &response[0], response.size(), 0);
      if (ret != response.size())
        LOG("failed to write control packet response");
    }
    close(fd);
  });
}

struct cap_header_struct {
  __u32 version;
  int pid;
};

struct cap_data_struct {
  __u32 effective;
  __u32 permitted;
  __u32 inheritable;
};

static int set_cap(int flags) {
  cap_header_struct capheader = {_LINUX_CAPABILITY_VERSION_1, 0};
  cap_data_struct capdata;
  capdata.inheritable = capdata.permitted = capdata.effective = flags;
  return (int)syscall(SYS_capset, &capheader, &capdata);
}

void dropCap(std::string configDir) {
  beforeDropCap();

  if (chroot(configDir.c_str()) < 0) {
    perror("chroot");
    exit(1);
  }
  if (chdir("/") < 0) {
    perror("chdir");
    exit(1);
  }
  configDir = "/";

  if (prctl(PR_SET_SECUREBITS, SECBIT_KEEP_CAPS | SECBIT_NOROOT) < 0) {
    perror("prctl");
    exit(1);
  }
  if (set_cap(0) < 0) {
    perror("set_cap");
    exit(1);
  }
}

int originalNetns = -1;

void switchNetns(const char* ns) {
  originalNetns = open("/proc/self/ns/net", O_RDONLY);
  if (originalNetns == -1) {
    LOG("failed to open origin netns");
    exit(1);
  }

  int newFd = open(ns, O_RDONLY);
  if (originalNetns == -1) {
    LOG("failed to open netns");
    exit(1);
  }

  if (setns(newFd, CLONE_NEWNET) != 0) {
    LOG("failed to switch netns");
    exit(0);
  }
  close(newFd);
}

void revertNetns() {
  if (originalNetns != -1) {
    if (setns(originalNetns, 0) != 0) {
      LOG("failed to switch netns back");
      exit(1);
    }

    close(originalNetns);
  }
}

void iptablesInsert(std::string rule) {
  if (system(("ip6tables --check " + rule + "|| ip6tables --append " + rule)
                 .c_str()) != 0) {
    LOG("setting iptables failed");
    exit(1);
  }
  FileStorage::saveIp6tablesRuleForDeletion("/var/lib/husarnet/", rule);
}

void systemdNotify() {
  const char* msg = "READY=1";
  const char* sockPath = getenv("NOTIFY_SOCKET");
  if (sockPath != NULL) {
    sockaddr_un un;
    un.sun_family = AF_UNIX;
    assert(strlen(sockPath) < sizeof(un.sun_path) - 1);

    memcpy(&un.sun_path[0], sockPath, strlen(sockPath) + 1);

    if (un.sun_path[0] == '@') {
      un.sun_path[0] = '\0';
    }

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    assert(fd != 0);
    sendto(fd, msg, strlen(msg), MSG_NOSIGNAL, (sockaddr*)(&un), sizeof(un));
    close(fd);
  }
}

std::string prepareConfigDir() {
  const char* confdir = getenv("HUSARNET_CONFIG");
  if (confdir == nullptr)
    confdir = "/var/lib/husarnet/";
  std::string configDir = confdir;
  if (configDir.back() != '/') {
    configDir += '/';
  }

  mkdir(configDir.c_str(), 0700);
  chmod(configDir.c_str(), 0700);

  return configDir;
}

void serviceMain(bool doFork = false) {
  signal(SIGPIPE, SIG_IGN);

  const char* netns = getenv("HUSARNET_NETNS");
  if (netns) {
    switchNetns(netns);
  }

  std::string configDir = prepareConfigDir();

  if (access(FileStorage::idFilePath(configDir).c_str(), R_OK) != 0) {
    FileStorage::generateAndWriteId(configDir);
  }
  if (access(FileStorage::httpSecretFilePath(configDir).c_str(), R_OK) != 0) {
    FileStorage::generateAndWriteHttpSecret(configDir);
  }

  if (chdir(configDir.c_str()) != 0) {
    LOG("failed to chdir to config dir");
    exit(1);
  }
  std::string httpSecret = FileStorage::readHttpSecret(configDir);
  configDir = "./";

  ServiceHelper::startServiceHelperProc(configDir);

  if (system("modprobe ip6_tables 2>/dev/null; modprobe tun 2>/dev/null") !=
      0) {
    // ignore
  }

  iptablesInsert("INPUT -s fc94::/16 -i lo -j ACCEPT");
  iptablesInsert("INPUT -s fc94::/16 \\! -i hnet0 -j DROP");

  Identity* identity;
  identity = tryCreateSmartcardIdentity();
  bool usingSmartcard;
  if (!identity) {
    usingSmartcard = false;
    identity = FileStorage::readIdentity(configDir);
  } else {
    usingSmartcard = true;
  }

  if (identity->deviceId ==
      BadDeviceId) {  // we had a bug where bad IDs would be generated
    FileStorage::generateAndWriteId(configDir);
    identity = FileStorage::readIdentity(configDir);
  }
  // if license not found -> pull the default one from app.husarnet.com
  BaseConfig* baseConfig;
  auto path = FileStorage::licenseFilePath(configDir);
  std::ifstream input(path);
  if (input.is_open()) {
    std::string str((std::istreambuf_iterator<char>(input)),
                    std::istreambuf_iterator<char>());
    baseConfig = new BaseConfig(str);
  } else {
    // pull license
    IpAddress ip = OsSocket::resolveToIp(::dashboardHostname);
    InetAddress address{ip, 80};
    std::string license = retrieveLicense(address);

    // read new license
    std::ifstream inputDefault(path);
    if (inputDefault.is_open()) {
      std::string str((std::istreambuf_iterator<char>(input)),
                      std::istreambuf_iterator<char>());
      baseConfig = new BaseConfig(license);
    } else {
      LOG("failed to pull the default license");
      exit(1);  // TODO (?): remove hardcoded IPs from husarnet_config.h
    }
  }

  NgSocket* sock = NgSocketSecure::create(identity, baseConfig);

  Settings settings(FileStorage::settingsFilePath(configDir));
  settings.loadInto(sock->options);

  if (usingSmartcard) {
    std::string ua = sock->options->userAgent;
    if (ua != "" && ua.back() != '\n')
      ua += "\n";
    ua += "smartcard\n";
  }
  LogManager* logManager = new LogManager(100);
  globalLogManager = logManager;
  ConfigTable* configTable = createSqliteConfigTable(configDir + "config.db");
  ConfigManager configManager(identity, baseConfig, configTable,
                              ServiceHelper::updateHostsFile, sock, httpSecret,
                              logManager);

  sock->options->isPeerAllowed = [&](DeviceId id) {
    return configManager.isPeerAllowed(id);
  };

  sock->options->userAgent = "Linux daemon";

  configManager.shouldSendInitRequest =
      getenv("HUSARNET_SEND_INIT_REQUEST") != nullptr;

  if (system("[ -e /dev/net/tun ] || (mkdir -p /dev/net; mknod /dev/net/tun c "
             "10 200)") != 0) {
    LOG("failed to create TUN device");
  }

  NgSocket* wrappedSock = sock;
  NgSocket* netDev = NetworkDev::wrap(
      identity->deviceId, wrappedSock,
      [&](DeviceId id) { return configManager.getMulticastDestinations(id); });

  std::string myIp = IpAddress::fromBinary(identity->deviceId).str();

  bool useTap = getenv("HUSARNET_USE_TAP") != nullptr;
  if (useTap) {
    TunDelegate::startTun("hnet0", L2Unwrapper::wrap(netDev), /*isTap=*/true);

    std::string gateway = L2Unwrapper::ipAddr;
    std::string macAddr = "52:54:00:fc:94:4d";
    if (system("ip link set hnet0 address 52:54:00:fc:94:4c") != 0 ||
        system(("ip neighbour add " + gateway + " lladdr " + macAddr +
                " dev hnet0 nud permanent")
                   .c_str()) != 0 ||
        system(("ip route add " + gateway + "/128 dev hnet0").c_str()) != 0 ||
        system(("ip route add fc94::/16 via " + gateway + "").c_str()) != 0) {
      LOG("failed to set up route");
      exit(1);
    }

    if (system("ip link set dev hnet0 mtu 1350") != 0 ||
        system(("ip addr add dev hnet0 " + myIp).c_str()) != 0 ||
        system("ip link set dev hnet0 up") != 0) {
      LOG("failed to setup IP address");
      exit(1);
    }
  } else {
    TunDelegate::startTun("hnet0", netDev);
    // 1350 - minimum required to tunnel IPv6 in VXLAN over Husarnet
    // Overheads: 50 bytes - Husarnet + 28 bytes UDP = 1428 bytes
    // In future, we would like to have internal fragmentation support or PMTU
    // discovery.

    if (system("sysctl net.ipv6.conf.lo.disable_ipv6=0") != 0 ||
        system("sysctl net.ipv6.conf.hnet0.disable_ipv6=0") != 0) {
      LOG("failed to enable IPv6 (may be harmless)");
    }

    if (system("ip link set dev hnet0 mtu 1350") != 0 ||
        system(("ip addr add dev hnet0 " + myIp + "/16").c_str()) != 0 ||
        system("ip link set dev hnet0 up") != 0) {
      LOG("failed to setup IP address");
      exit(1);
    }
  }

  if (system("ip -6 route add ff15:f2d3:a389::/48 dev hnet0 table local") !=
      0) {
    LOG("failed to setup multicast route");
  }

  // important: no messages will be processed until we call `runHusarnet`
  bindControlSocket(configManager, FileStorage::controlSocketPath(configDir));

  systemdNotify();

  revertNetns();

  if (doFork) {
    int pid = fork();
    if (pid < 0) {
      perror("fork");
      _exit(1);
    }

    if (pid != 0)
      _exit(0);
  }

  dropCap(configDir);

  configTable->open();  // this needs to be after fork, or Sqlite complains
  LegacyFileStorage::migrateToConfigTable(configTable, configDir);
  configManager.updateHosts();

  configManager.runHusarnet();
}
