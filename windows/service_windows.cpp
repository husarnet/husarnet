#include <fstream>
#include "configmanager.h"
#include "filestorage.h"
#include "global_lock.h"
#include "husarnet.h"
#include "husarnet_crypto.h"
#include "husarnet_secure.h"
#include "l2unwrapper.h"
#include "legacy_filestorage.h"
#include "network_dev.h"
#include "port.h"
#include "service_helper.h"
#include "shlwapi.h"
#include "sockets.h"
#include "sqlite_configtable.h"
#include "tap_windows.h"
#include "util.h"

static std::string configDir;

class WinTapDelegate : public NgSocketDelegate {
  WinTap* tap;

 public:
  WinTapDelegate(WinTap* tap) : tap(tap) {}

  void onDataPacket(DeviceId source, string_view data) { tap->write(data); }
};

int mySystem(std::string cmd) {
  return system(("\"" + cmd + "\"").c_str());
}

std::string getControlSharedSecret(std::string configDir) {
  std::ifstream f(configDir + "/control_shared_secret");
  std::string sharedSecret;
  f >> sharedSecret;
  if (sharedSecret.size() != 24) {
    LOG("create new control shared secret");
    std::ofstream f(configDir + "/control_shared_secret");
    sharedSecret = encodeHex(randBytes(12));
    f << sharedSecret;
  }
  return sharedSecret;
}

std::string handleControlPacket(ConfigManager& manager, std::string data) {
  std::string sharedSecret = getControlSharedSecret(configDir);
  assert(sharedSecret.size() > 0);

  if (data.size() < sharedSecret.size() ||
      !NgSocketCrypto::safeEquals(data.substr(0, sharedSecret.size()),
                                  sharedSecret)) {
    return "";
  }
  data = data.substr(sharedSecret.size());
  return manager.handleControlPacket(data);
}

void handleControlConnection(ConfigManager& manager, int fd) {
  std::string buf;
  buf.resize(1024);
  long ret = ::recv(fd, &buf[0], buf.size(), 0);

  if (ret < 0) {
    LOG("failed to receive control packet");
    return;
  }

  GIL::lock();
  std::string response = handleControlPacket(manager, buf.substr(0, ret));
  GIL::unlock();

  if (response.size() > 0) {
    long ret = ::send(fd, &response[0], response.size(), 0);
    if (ret != response.size())
      LOG("failed to write control packet response");

    shutdown(fd, SD_SEND);
  }
}

void bindControlSocket(ConfigManager& manager) {
  sockaddr_in6 servaddr =
      OsSocket::sockaddrFromIp(InetAddress::parse("[::1]:5579"));

  int sockfd =
      socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // why AF_INET6 doesn't work?
  if (bind(sockfd, (sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
    LOG("failed to bind control socket (err = %d)", WSAGetLastError());
    abort();
  }
  if (listen(sockfd, SOMAXCONN) != 0)
    abort();

  startThread(
      [sockfd, &manager]() {
        while (true) {
          int fd = accept(sockfd, nullptr, nullptr);
          if (fd < 0) {
            LOG("control socket accept failed");
            return;
          }

          startThread(
              [fd, &manager]() { handleControlConnection(manager, fd); },
              "control", 0);
        }
      },
      "control", 0);
}

std::string getConfigDir() {
  return std::string(getenv("PROGRAMDATA")) + "/husarnet/";
}

std::string getSavedDeviceName() {
  std::string name;
  std::ifstream f(getConfigDir() + "/saved-tun-name.txt");
  if (f.good())
    std::getline(f, name);

  return name;
}

void fixPermissions(std::string path) {
  // https://superuser.com/questions/116379/setting-chmod-700-with-windows-7-permission
  system(("icacls \"" + path +
          "\" /grant:r *S-1-5-18:(F) *S-1-3-0:(F) *S-1-5-32-544:(F) "
          "/inheritance:r")
             .c_str());
  system(("icacls \"" + path +
          "\" /grant:r *S-1-5-18:(OI)(CI)F *S-1-3-0:(OI)(CI)F "
          "*S-1-5-32-544:(OI)(CI)F /inheritance:r")
             .c_str());
  // S-1-5-18 is local system
  // S-1-5-32-544 - administrators
}

void saveDeviceName(std::string name) {
  std::ofstream f(getConfigDir() + "/saved-tun-name.txt");
  f << name << std::endl;
}

void serviceMain() {
  configDir = getConfigDir();

  if (!PathFileExists(FileStorage::idFilePath(configDir).c_str())) {
    CreateDirectory(configDir.c_str(), NULL);
    // fixPermissions(configDir);
    FileStorage::generateAndWriteId(configDir);
  }

  if (!PathFileExists(FileStorage::httpSecretFilePath(configDir).c_str())) {
    FileStorage::generateAndWriteHttpSecret(configDir);
  }

  std::string httpSecret = FileStorage::readHttpSecret(configDir);

  auto identity = FileStorage::readIdentity(configDir);

  BaseConfig* baseConfig = BaseConfig::create(configDir);
  NgSocket* sock = NgSocketSecure::create(identity, baseConfig);

  ServiceHelper::startServiceHelperProc(configDir);

  ConfigTable* configTable = createSqliteConfigTable(configDir + "config.db");
  configTable->open();
  LegacyFileStorage::migrateToConfigTable(configTable, configDir);

  LogManager* logManager = new LogManager(100);
  globalLogManager = logManager;
  ConfigManager configManager(identity, baseConfig, configTable,
                              ServiceHelper::updateHostsFile, sock, httpSecret,
                              logManager);

  sock->options->isPeerAllowed = [&](DeviceId id) {
    return configManager.isPeerAllowed(id);
  };

  sock->options->userAgent = "Windows daemon";

  NgSocket* wrappedSock = sock;
  NgSocket* netDevL3 = NetworkDev::wrap(
      identity->deviceId, wrappedSock,
      [&](DeviceId id) { return configManager.getMulticastDestinations(id); });

  WinTap* winTap = WinTap::create(getSavedDeviceName());
  saveDeviceName(winTap->getDeviceName());
  winTap->bringUp();

  std::string sourceNetshName = winTap->getNetshName();
  std::string netshName = "Husarnet";
  if (netshName != sourceNetshName) {
    if (mySystem("netsh interface set interface name = \"" + sourceNetshName +
                 "\" newname = \"" + netshName + "\"") == 0) {
      LOG("renamed successfully");
    } else {
      netshName = sourceNetshName;
      LOG("rename failed");
    }
  }
  std::string quotedName = "\"" + netshName + "\"";

  mySystem("netsh interface ipv6 add neighbors " + quotedName +
           " fc94:8385:160b:88d1:c2ec:af1b:06ac:0001 52-54-00-fc-94-4d");
  std::string myIp = IpAddress::fromBinary(identity->deviceId).str();
  mySystem("netsh interface ipv6 add address " + quotedName + " " + myIp +
           "/128");
  mySystem(
      "netsh interface ipv6 add route "
      "fc94:8385:160b:88d1:c2ec:af1b:06ac:0001/128 " +
      quotedName);
  mySystem("netsh interface ipv6 add route fc94::/16 " + quotedName +
           " fc94:8385:160b:88d1:c2ec:af1b:06ac:0001");

  std::string cmd =
      "powershell New-NetFirewallRule -DisplayName AllowHusarnet -Direction "
      "Inbound -Action Allow -LocalAddress fc94::/16 -RemoteAddress fc94::/16 "
      "-InterfaceAlias \"\"\"" +
      netshName + "\"\"\"";
  mySystem((cmd).c_str());

  NgSocket* netDev = L2Unwrapper::wrap(netDevL3, winTap->getMac());

  GIL::startThread(
      [&] {
        std::string buf;
        buf.resize(4096);

        while (true) {
          string_view pkt = winTap->read(buf);

          netDev->sendDataPacket(BadDeviceId, pkt);
        }
      },
      "wintap-read");

  netDev->delegate = new WinTapDelegate(winTap);
  bindControlSocket(configManager);

  configManager.updateHosts();
  configManager.runHusarnet();
}
