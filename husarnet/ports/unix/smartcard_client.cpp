// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// #include <sys/socket.h>
// #include <sys/types.h>
// #include <sys/un.h>
// #include <fstream>
// #include "husarnet_crypto.h"
// #include "util.h"

// int connectToUnix(std::string path) {
//   sockaddr_un servaddr = {0};
//   memset(&servaddr, 0, sizeof(servaddr));
//   servaddr.sun_family = AF_UNIX;
//   strncpy(servaddr.sun_path, path.c_str(), sizeof(servaddr.sun_path) - 1);

//   int s = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
//   if (connect(s, (sockaddr*)&servaddr, sizeof(servaddr)) != 0)
//     return -1;
//   return s;
// }

// struct SmartcardIdentity : Identity {
//   bool trySign(const std::string& data, fstring<64>& out) {
//     int s = connectToUnix("smartcard.sock");
//     if (s == -1) {
//       return false;
//     }

//     std::string buf = "sign " + encodeHex(data);
//     if (write(s, (void*)buf.data(), buf.size()) != buf.size()) {
//       close(s);
//       return false;
//     }

//     if (read(s, out.data(), out.size()) != out.size()) {
//       close(s);
//       return false;
//     }

//     close(s);
//     return true;
//   }

//   fstring<64> sign(const std::string& data) override {
//     fstring<64> out;
//     while (true) {
//       bool ok = trySign(data, out);
//       if (!ok) {
//         LOG("failed to retrieve signature from the smartcard, retrying after
//         3 "
//             "seconds");
//         sleep(3);
//       }
//       break;
//     }
//     return out;
//   }
// };

// Identity* tryCreateSmartcardIdentity() {
//   std::ifstream input("smartcard-id");
//   if (input.good()) {
//     LOG("using smartcard");
//     std::string ip, pubkeyS, serialno;
//     input >> ip >> pubkeyS >> serialno;

//     auto identity = new SmartcardIdentity();
//     identity->pubkey = decodeHex(pubkeyS);
//     identity->deviceId = IpAddress::parse(ip.c_str()).toBinary();

//     identity->sign("test");
//     return identity;
//   }
//   return nullptr;
// }
