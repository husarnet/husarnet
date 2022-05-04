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
//   signal(SIGPIPE, SIG_IGN)

//   if (chdir(configDir.c_str()) != 0) {
//     LOG("failed to chdir to config dir");
//     exit(1);
//   }
//   configDir = "./";

//   ServiceHelper::startServiceHelperProc(configDir);

//   Settings settings(FileStorage::settingsFilePath(configDir));
//   settings.loadInto(sock->options);

//   systemdNotify();

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
