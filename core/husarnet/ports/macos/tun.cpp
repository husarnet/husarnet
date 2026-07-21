// Copyright (c) 2025 Husarnet sp. z o.o.
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

#include "husarnet/logging.h"
#include "husarnet/util.h"

static int createUtunSocket(uint32_t& ctlId)
{
  struct ctl_info ctlInfo;
  memset(&ctlInfo, 0, sizeof(ctlInfo));

  if(strlcpy(ctlInfo.ctl_name, UTUN_CONTROL_NAME, sizeof(ctlInfo.ctl_name)) >= sizeof(ctlInfo.ctl_name)) {
    HLOG_ERROR("UTUN_CONTROL_NAME too long");
    return -1;
  }

  int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
  if(fd == -1) {
    HLOG_ERROR("socket(PF_SYSTEM, SYSPROTO_CONTROL) failed // {error}", strerror(errno));
    return -1;
  }

  if(ioctl(fd, CTLIOCGINFO, &ctlInfo) == -1) {
    HLOG_ERROR("ioctl(CTLIOCGINFO) failed // {error}", strerror(errno));
    close(fd);
    return -1;
  }

  ctlId = ctlInfo.ctl_id;
  return fd;
}

// Asks the kernel for the interface name it actually assigned to this socket.
static bool queryUtunName(int fd, std::string& name)
{
  char ifname[IFNAMSIZ] = {};
  socklen_t ifnameLen = sizeof(ifname);

  if(getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifname, &ifnameLen) == -1) {
    HLOG_ERROR("getsockopt(UTUN_OPT_IFNAME) failed // {error}", strerror(errno));
    return false;
  }

  name = std::string(ifname, strnlen(ifname, sizeof(ifname)));
  return true;
}

// Opens utun device, letting the kernel pick the unit number.
static int openTun(std::string& name)
{
  uint32_t ctlId = 0;
  int fd = createUtunSocket(ctlId);
  if(fd == -1) {
    return -1;
  }

  struct sockaddr_ctl sc;
  memset(&sc, 0, sizeof(sc));
  sc.sc_id = ctlId;
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_unit = 0;  // 0 means the kernel will pick the unit number for us

  if(connect(fd, (struct sockaddr*)&sc, sizeof(sc)) == -1) {
    int connectErrno = errno;
    close(fd);

    switch(connectErrno) {
      case EPERM:
      case EACCES:
        HLOG_ERROR(
            "not permitted to create a utun device - is the daemon running as root? // {error}",
            strerror(connectErrno));
        break;
      case EBUSY:
        HLOG_ERROR("no free utun unit available on this system");
        break;
      default:
        HLOG_ERROR("connect(AF_SYS_CONTROL) failed // {error}", strerror(connectErrno));
    }
    return -1;
  }

  if(!queryUtunName(fd, name)) {
    close(fd);
    return -1;
  }

  return fd;
}

void Tun::close()
{
  SOCKFUNC_close(fd);
}

bool Tun::isRunning()
{
  return fd != -1;
}

void Tun::onTunData()
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
  sendToLowerLayer(IpAddress(), packet.substr(4));
}

Tun::Tun()
{
  tunBuffer.resize(4096);
  std::string tunName{};
  fd = openTun(tunName);
  this->name = tunName;

  if(fd == -1) {
    HLOG_ERROR("Can't open utun device!");
    return;
  }

  // onTunData() relies on EAGAIN to detect a spurious wakeup, which only ever
  // happens on a non-blocking fd.
  int flags = fcntl(fd, F_GETFL, 0);
  if(flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    HLOG_WARNING("failed to set utun fd non-blocking // {error}", strerror(errno));
  }

  HLOG_INFO("utun device opened successfully // {interface}", tunName.c_str());
  OsSocket::bindCustomFd(fd, std::bind(&Tun::onTunData, this));
}

void Tun::onLowerLayerData(HusarnetAddress source, string_view data)
{
  std::string wrapped{};
  // prepend bytes specific for utun/macos
  wrapped += string_view("\x00\x00\x00\x1e", 4);
  wrapped += data;

  long wr = write(fd, wrapped.c_str(), wrapped.size());
  if(wr != wrapped.size()) {
    HLOG_INFO("short tun write");
  }
}

std::string Tun::getName()
{
  return this->name;
}
