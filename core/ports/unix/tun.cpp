// // Copyright (c) 2017 Husarion Sp. z o.o.
// // author: Michał Zieliński (zielmicha)
// #include "tun.h"
// #include <linux/if_tun.h>
// #include <net/if.h>
// #include <signal.h>
// #include <sys/ioctl.h>
// #include <fstream>
// #include <iostream>
// #include "husarnet.h"
// #include "husarnet_secure.h"
// #include "network_dev.h"
// #include "port.h"
// #include "sockets.h"
// #include "util.h"

// TunDelegate::TunDelegate(NgSocket* sock) : sock(sock) {
//   tunBuffer.resize(4096);
//   sock->delegate = this;
// }

// void TunDelegate::tunReady() {
//   long size = read(tunFd, &tunBuffer[0], tunBuffer.size());

//   if (size <= 0) {
//     if (errno == EAGAIN) {
//       return;
//     } else {
//       tunFd = -1;
//       return;
//     }
//   }

//   string_view packet = string_view(tunBuffer).substr(0, size);
//   sock->sendDataPacket(BadDeviceId, packet);
// }

// void TunDelegate::onDataPacket(DeviceId source, string_view data) {
//   long wr = write(tunFd, data.data(), data.size());
//   if (wr != data.size())
//     LOG("short tun write");
// }

// int openTun(std::string name, bool isTap) {
//   struct ifreq ifr;
//   int fd;

//   if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
//     perror("open /dev/net/tun");
//     exit(1);
//   }

//   memset(&ifr, 0, sizeof(ifr));
//   ifr.ifr_flags = (isTap ? IFF_TAP : IFF_TUN) | IFF_NO_PI;
//   assert(name.size() < IFNAMSIZ);
//   strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
//   if (ioctl(fd, TUNSETIFF, (void*)&ifr) < 0) {
//     perror("TUNSETIFF");
//     exit(1);
//   }

//   LOG("allocated tun %s", ifr.ifr_name);

//   return fd;
// }

// void TunDelegate::setFd(int tunFd) {
//   this->tunFd = tunFd;
//   OsSocket::bindCustomFd(tunFd, std::bind(&TunDelegate::tunReady, this));
// }

// void TunDelegate::closeFd() {
//   close(this->tunFd);
// }

// bool TunDelegate::isRunning() {
//   return tunFd != -1;
// }

// TunDelegate* TunDelegate::createTun(NgSocket* netDev) {
//   return new TunDelegate(netDev);
// }

// TunDelegate* TunDelegate::startTun(std::string name,
//                                    NgSocket* netDev,
//                                    bool isTap) {
//   int fd = openTun(name, isTap);
//   TunDelegate* delegate = createTun(netDev);
//   delegate->setFd(fd);
//   return delegate;
// }
