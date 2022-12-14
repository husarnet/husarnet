// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/unix/tun.h"

#include <functional>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <net/if_utun.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/util.h"

inline int utun_open(std::string& name, const int unit)
{
  struct sockaddr_ctl sc;
  struct ctl_info ctlInfo;

  memset(&ctlInfo, 0, sizeof(ctlInfo));
  if (strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name))
      >= sizeof(ctlInfo.ctl_name))
    LOG("UTUN_CONTROL_NAME too long");

  ScopedFD fd(socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL));
  if (!fd.defined())
    LOG("socket(SYSPROTO_CONTROL)");

  if (ioctl(fd(), CTLIOCGINFO, &ctlInfo) == -1)
    LOG("ioctl(CTLIOCGINFO)");

  sc.sc_id = ctlInfo.ctl_id;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_unit = unit + 1;
  std::memset(sc.sc_reserved, 0, sizeof(sc.sc_reserved));

  // If the connect is successful, a utunX device will be created, where X
  // is our unit number - 1.
  if (connect(fd(), (struct sockaddr *)&sc, sizeof(sc)) == -1)
    return -1;

  // Get iface name of newly created utun dev.
  char utunname[20];
  socklen_t utunname_len = sizeof(utunname);
  if (getsockopt(fd(), SYSPROTO_CONTROL, UTUN_OPT_IFNAME, utunname, &utunname_len))
    LOG("getsockopt(SYSPROTO_CONTROL)");
  name = utunname;

  return fd.release();
}

// Try to open an available utun device unit.
// Return the iface name in name.
inline int utun_open(std::string& name)
{
  for (int unit = 0; unit < 256; ++unit)
  {
    const int fd = utun_open(name, unit);
    if (fd >= 0)
      return fd;
  }
  LOG("cannot open available utun device");
}

static int openTun(std::string name, bool isTap)
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

  LOG("allocated tun %s", ifr.ifr_name);

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
    LOG("short tun write");
  }
}
