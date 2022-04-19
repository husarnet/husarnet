// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// TODO remove this whole file

// struct cap_header_struct {
//   __u32 version;
//   int pid;
// };

// struct cap_data_struct {
//   __u32 effective;
//   __u32 permitted;
//   __u32 inheritable;
// };

// static int set_cap(int flags) {
//   cap_header_struct capheader = {_LINUX_CAPABILITY_VERSION_1, 0};
//   cap_data_struct capdata;
//   capdata.inheritable = capdata.permitted = capdata.effective = flags;
//   return (int)syscall(SYS_capset, &capheader, &capdata);
// }

// void dropCap(std::string configDir) {
//   beforeDropCap();

//   if (chroot(configDir.c_str()) < 0) {
//     perror("chroot");
//     exit(1);
//   }
//   if (chdir("/") < 0) {
//     perror("chdir");
//     exit(1);
//   }
//   configDir = "/";

//   if (prctl(PR_SET_SECUREBITS, SECBIT_KEEP_CAPS | SECBIT_NOROOT) < 0) {
//     perror("prctl");
//     exit(1);
//   }
//   if (set_cap(0) < 0) {
//     perror("set_cap");
//     exit(1);
//   }
// }

// static int originalNetns = -1;

// void switchNetns(const char* ns) {
//   originalNetns = open("/proc/self/ns/net", O_RDONLY);
//   if (originalNetns == -1) {
//     LOG("failed to open origin netns");
//     exit(1);
//   }

//   int newFd = open(ns, O_RDONLY);
//   if (originalNetns == -1) {
//     LOG("failed to open netns");
//     exit(1);
//   }

//   if (setns(newFd, CLONE_NEWNET) != 0) {
//     LOG("failed to switch netns");
//     exit(0);
//   }
//   close(newFd);
// }

// void revertNetns() {
//   if (originalNetns != -1) {
//     if (setns(originalNetns, 0) != 0) {
//       LOG("failed to switch netns back");
//       exit(1);
//     }

//     close(originalNetns);
//   }
// }

// void systemdNotify() {
//   const char* msg = "READY=1";
//   const char* sockPath = getenv("NOTIFY_SOCKET");
//   if (sockPath != NULL) {
//     sockaddr_un un;
//     un.sun_family = AF_UNIX;
//     assert(strlen(sockPath) < sizeof(un.sun_path) - 1);

//     memcpy(&un.sun_path[0], sockPath, strlen(sockPath) + 1);

//     if (un.sun_path[0] == '@') {
//       un.sun_path[0] = '\0';
//     }

//     int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
//     assert(fd != 0);
//     sendto(fd, msg, strlen(msg), MSG_NOSIGNAL, (sockaddr*)(&un), sizeof(un));
//     close(fd);
//   }
// }

// void serviceMain(bool doFork = false) {
//   signal(SIGPIPE, SIG_IGN);

//   const char* netns = getenv("HUSARNET_NETNS");
//   if (netns) {
//     switchNetns(netns);
//   }

//   if (chdir(configDir.c_str()) != 0) {
//     LOG("failed to chdir to config dir");
//     exit(1);
//   }
//   configDir = "./";

//   ServiceHelper::startServiceHelperProc(configDir);

//   Settings settings(FileStorage::settingsFilePath(configDir));
//   settings.loadInto(sock->options);

//   if (usingSmartcard) {
//     std::string ua = sock->options->userAgent;
//     if (ua != "" && ua.back() != '\n')
//       ua += "\n";
//     ua += "smartcard\n";
//   }
//   LogManager* logManager = new LogManager(100);
//   globalLogManager = logManager;

//   sock->options->isPeerAllowed = [&](DeviceId id) {
//     return configManager.isPeerAllowed(id);
//   };

//   sock->options->userAgent = "Linux daemon";

//   configManager.shouldSendInitRequest =
//       getenv("HUSARNET_SEND_INIT_REQUEST") != nullptr;

//   if (system("[ -e /dev/net/tun ] || (mkdir -p /dev/net; mknod /dev/net/tun c
//   "
//              "10 200)") != 0) {
//     LOG("failed to create TUN device");
//   }

//   NgSocket* wrappedSock = sock;
//   NgSocket* netDev = NetworkDev::wrap(
//       identity->deviceId, wrappedSock,
//       [&](DeviceId id) { return configManager.getMulticastDestinations(id);
//       });

//   std::string myIp = IpAddress::fromBinary(identity->deviceId).str();

//   bool useTap = getenv("HUSARNET_USE_TAP") != nullptr;
//   if (useTap) {
//     TunDelegate::startTun("hnet0", L2Unwrapper::wrap(netDev),
//     /*isTap=*/true);

//     std::string gateway = L2Unwrapper::ipAddr;
//     std::string macAddr = "52:54:00:fc:94:4d";
//     if (system("ip link set hnet0 address 52:54:00:fc:94:4c") != 0 ||
//         system(("ip neighbour add " + gateway + " lladdr " + macAddr +
//                 " dev hnet0 nud permanent")
//                    .c_str()) != 0 ||
//         system(("ip route add " + gateway + "/128 dev hnet0").c_str()) != 0
//         || system(("ip route add fc94::/16 via " + gateway + "").c_str()) !=
//         0) {
//       LOG("failed to set up route");
//       exit(1);
//     }

//     if (system("ip link set dev hnet0 mtu 1350") != 0 ||
//         system(("ip addr add dev hnet0 " + myIp).c_str()) != 0 ||
//         system("ip link set dev hnet0 up") != 0) {
//       LOG("failed to setup IP address");
//       exit(1);
//     }
//   } else {
//     TunDelegate::startTun("hnet0", netDev);
//     // 1350 - minimum required to tunnel IPv6 in VXLAN over Husarnet
//     // Overheads: 50 bytes - Husarnet + 28 bytes UDP = 1428 bytes
//     // In future, we would like to have internal fragmentation support or
//     PMTU
//     // discovery.

//     if (system("sysctl net.ipv6.conf.lo.disable_ipv6=0") != 0 ||
//         system("sysctl net.ipv6.conf.hnet0.disable_ipv6=0") != 0) {
//       LOG("failed to enable IPv6 (may be harmless)");
//     }

//     if (system("ip link set dev hnet0 mtu 1350") != 0 ||
//         system(("ip addr add dev hnet0 " + myIp + "/16").c_str()) != 0 ||
//         system("ip link set dev hnet0 up") != 0) {
//       LOG("failed to setup IP address");
//       exit(1);
//     }
//   }

//   if (system("ip -6 route add ff15:f2d3:a389::/48 dev hnet0 table local") !=
//       0) {
//     LOG("failed to setup multicast route");
//   }

//   systemdNotify();

//   revertNetns();

//   if (doFork) {
//     int pid = fork();
//     if (pid < 0) {
//       perror("fork");
//       _exit(1);
//     }

//     if (pid != 0)
//       _exit(0);
//   }

//   dropCap(configDir);

//   configTable->open();  // this needs to be after fork, or Sqlite complains
//   LegacyFileStorage::migrateToConfigTable(configTable, configDir);
//   configManager.updateHosts();

//   configManager.runHusarnet();
// }
