// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/macos/tun.h"

#include <functional>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "husarnet/ports/macos/types.h"
#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/util.h"

static int openTun(std::string& name)
{
  struct sockaddr_ctl sc;
  struct ctl_info ctlInfo;
  int fd;

  memset(&ctlInfo, 0, sizeof(ctlInfo));
  if(strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name)) >=
     sizeof(ctlInfo.ctl_name)) {
    fprintf(stderr, "UTUN_CONTROL_NAME too long");
    return -1;
  }
  fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);

  if(fd == -1) {
    perror("socket(SYSPROTO_CONTROL)");
    return -1;
  }
  if(ioctl(fd, CTLIOCGINFO, &ctlInfo) == -1) {
    perror("ioctl(CTLIOCGINFO)");
    close(fd);
    return -1;
  }

  sc.sc_id = ctlInfo.ctl_id;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;

  // If the connect is successful, a tun%d device will be created, where "%d"
  // is our unit number -1
  int unitNumber = 1;

  // bruteforce... let's try until we hit free utun unit number:)
  while(unitNumber < 10) {
    sc.sc_unit = unitNumber;
    if(connect(fd, (struct sockaddr*)&sc, sizeof(sc)) == -1) {
      LOG("Can't bind to utun%d", unitNumber - 1);
      // perror ("connect(AF_SYS_CONTROL)");
      unitNumber++;
    } else {
      name = "utun" + std::to_string(unitNumber - 1);
      return fd;
    }
  }
  close(fd);
  return -1;
}

void TunTap::close()
{
  SOCKFUNC_close(fd);
}

bool TunTap::isRunning()
{
  return fd != -1;
}

void TunTap::onTunTapData()
{
  long size = read(fd, &tunBuffer[0], tunBuffer.size());

  if(size <= 0) {
    if(errno == EAGAIN) {
      return;
    } else {
      fd = -1;
      return;
    }
  }

  string_view packet = string_view(tunBuffer).substr(0, size);
  sendToLowerLayer(BadDeviceId, packet.substr(4));
}

TunTap::TunTap()
{
  tunBuffer.resize(4096);
  std::string tunName{};
  fd = openTun(tunName);
  name = tunName;
  if(fd == -1) {
    LOG("Can't open utun device!");
  } else {
    LOG("utun device opened successfully");
  }
  OsSocket::bindCustomFd(fd, std::bind(&TunTap::onTunTapData, this));
}

void TunTap::onLowerLayerData(DeviceId source, string_view data)
{
  std::string wrapped{};
  // prepend bytes specific for utun/macos
  wrapped += string_view("\x00\x00\x00\x1e", 4);
  wrapped += data;

  long wr = write(fd, wrapped.c_str(), wrapped.size());
  if(wr != wrapped.size()) {
    LOG("short tun write");
  }
}

std::string TunTap::getName()
{
  return name;
}
