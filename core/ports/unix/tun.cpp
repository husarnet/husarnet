// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/unix/tun.h"

#include <functional>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

static int openTun(const std::string& name, bool isTap)
{
  struct ifreq ifr;
  int fd;

  if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("open /dev/net/tun");
    exit(1);
  }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = (isTap ? IFF_TAP : IFF_TUN) | IFF_NO_PI;
  assert(name.size() < IFNAMSIZ);
  strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
  if(ioctl(fd, TUNSETIFF, (void*)&ifr) < 0) {
    perror("TUNSETIFF");
    exit(1);
  }

  LOG_INFO("allocated tun %s", ifr.ifr_name);

  return fd;
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
  sendToLowerLayer(BadDeviceId, packet);
}

TunTap::TunTap(std::string name, bool isTap)
{
  tunBuffer.resize(4096);

  fd = openTun(name, isTap);
  OsSocket::bindCustomFd(fd, std::bind(&TunTap::onTunTapData, this));
}

void TunTap::onLowerLayerData(DeviceId source, string_view data)
{
  long wr = write(fd, data.data(), data.size());
  if(wr != data.size()) {
    LOG_INFO("short tun write");
  }
}
