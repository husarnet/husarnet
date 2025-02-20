// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/windows/tun.h"

#include <functional>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"
#include "husarnet/ports/windows/networking.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

// From OpenVPN tap driver, common.h
#define TAP_CONTROL_CODE(request, method) \
  CTL_CODE(FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)
#define TAP_IOCTL_GET_MAC TAP_CONTROL_CODE(1, METHOD_BUFFERED)
#define TAP_IOCTL_GET_VERSION TAP_CONTROL_CODE(2, METHOD_BUFFERED)
#define TAP_IOCTL_GET_MTU TAP_CONTROL_CODE(3, METHOD_BUFFERED)
#define TAP_IOCTL_GET_INFO TAP_CONTROL_CODE(4, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT TAP_CONTROL_CODE(5, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS TAP_CONTROL_CODE(6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ TAP_CONTROL_CODE(7, METHOD_BUFFERED)
#define TAP_IOCTL_GET_LOG_LINE TAP_CONTROL_CODE(8, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT TAP_CONTROL_CODE(9, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_TUN TAP_CONTROL_CODE(10, METHOD_BUFFERED)

static const std::string peerMacAddr = decodeHex("525400fc944d");
static const std::string defaultSelfMacAddr = decodeHex("525400fc944c");

static HANDLE openTun(const std::string& name)
{
  HANDLE tun_fd;

  std::string path = "\\\\.\\Global\\" + name + ".tap";
  tun_fd = CreateFile(
      path.c_str(), GENERIC_WRITE | GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
      FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);
  if(tun_fd == INVALID_HANDLE_VALUE) {
    int errcode = GetLastError();

    LOG_ERROR("failed to open tap device: %d", errcode);
    return INVALID_HANDLE_VALUE;
  }
  LOG_INFO("success: tap device opened");

  return tun_fd;
}

void TunTap::close()
{
  CloseHandle(tap_fd);
}

bool TunTap::isRunning()
{
  return tap_fd != INVALID_HANDLE_VALUE;
}

void TunTap::onTunTapData()
{
  // This is NOOP, as reading from TunTap is handled by separate
  // thread on Windows. We don't handle it via bindCustomFd and select()
}

void TunTap::startReaderThread()
{
  Port::threadStart(
      [&] {
        std::string buf;
        buf.resize(4096);

        threadMutex.lock();

        while(true) {
          string_view packet = read(buf);

          if(packet.substr(0, 6) == peerMacAddr &&
             packet.substr(6, 6) == selfMacAddr &&
             packet.substr(12, 2) == string_view("\x86\xdd", 2)) {
            sendToLowerLayer(IpAddress(), packet.substr(14));
          }
        }
      },
      "wintap-read");
}

TunTap::TunTap(HusarnetManager* manager, std::string name, bool isTap)
{
  (void)isTap;
  tap_fd = openTun(name);
  tunBuffer.resize(4096);

  bringUp();

  WindowsNetworking windowsNetworking;
  windowsNetworking.setupNetworkInterface(manager, name);
  windowsNetworking.allowHusarnetThroughWindowsFirewall("AllowHusarnet");

  selfMacAddr = getMac();
  startReaderThread();
}

string_view TunTap::read(std::string& buffer)
{
  threadMutex.unlock();

  OVERLAPPED overlapped = {0, 0, {{0}}, 0};
  overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  if(ReadFile(tap_fd, &buffer[0], (DWORD)buffer.size(), NULL, &overlapped) ==
     0) {
    if(GetLastError() != ERROR_IO_PENDING) {
      LOG_ERROR("tap read failed %d", (int)GetLastError());
      assert(false);
    }
  }

  WaitForSingleObject(overlapped.hEvent, INFINITE);
  CloseHandle(overlapped.hEvent);

  threadMutex.lock();

  int len = overlapped.InternalHigh;
  return string_view(buffer).substr(0, len);
}

void TunTap::write(string_view data)
{
  threadMutex.unlock();

  OVERLAPPED overlapped = {0, 0, {{0}}, 0};
  overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if(WriteFile(tap_fd, data.data(), (DWORD)data.size(), NULL, &overlapped)) {
    auto error = GetLastError();
    if(error != ERROR_IO_PENDING) {
      LOG_ERROR("tap write failed");
    }
  }

  WaitForSingleObject(overlapped.hEvent, INFINITE);
  CloseHandle(overlapped.hEvent);

  threadMutex.lock();
}

void TunTap::bringUp()
{
  DWORD len;

  ULONG flag = 1;
  if(DeviceIoControl(
         tap_fd, TAP_IOCTL_SET_MEDIA_STATUS, &flag, sizeof(flag), &flag,
         sizeof(flag), &len, NULL) == 0) {
    LOG_ERROR("failed to bring up the tap device");
    return;
  }

  LOG_INFO("tap config OK");
}

std::string TunTap::getMac()
{
  std::string hwaddr;
  hwaddr.resize(6);
  DWORD len;

  if(DeviceIoControl(
         tap_fd, TAP_IOCTL_GET_MAC, &hwaddr[0], hwaddr.size(), &hwaddr[0],
         hwaddr.size(), &len, NULL) == 0) {
    LOG_ERROR("failed to retrieve MAC address");
    assert(false);
  } else {
    LOG_DEBUG(
        "MAC address: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", 0xFF & hwaddr[0],
        0xFF & hwaddr[1], 0xFF & hwaddr[2], 0xFF & hwaddr[3], 0xFF & hwaddr[4],
        0xFF & hwaddr[5]);
  }
  return hwaddr;
}

void TunTap::onLowerLayerData(DeviceId source, string_view data)
{
  (void)source;
  std::string wrapped;

  wrapped += selfMacAddr;
  wrapped += peerMacAddr;
  wrapped += string_view("\x86\xdd", 2);
  wrapped += data;
  write(wrapped);
}
