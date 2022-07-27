// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/windows/tun.h"

#include <functional>

// TODO ympek chec unused imports...
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/gil.h"
#include "husarnet/identity.h"
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

static std::string getNetshNameForGuid(std::string guid)
{
  std::string path =
      "SYSTEM\\CurrentControlSet\\Control\\Network\\{4d36e972-e325-11ce-bfc1-"
      "08002be10318}\\" +
      guid + "\\connection";

  DWORD dwType = REG_SZ;
  HKEY hKey = 0;
  char value[1024];
  DWORD value_length = 1024;
  RegOpenKey(HKEY_LOCAL_MACHINE, path.c_str(), &hKey);
  RegQueryValueEx(hKey, "name", NULL, &dwType, (LPBYTE)&value, &value_length);

  return value;
}

static int mySystem(std::string cmd)
{
  return system(("\"" + cmd + "\"").c_str());
}

static HANDLE openTun(std::string name)
{
  HANDLE tun_fd;

  std::string path = "\\\\.\\Global\\" + name + ".tap";
  tun_fd = CreateFile(
      path.c_str(), GENERIC_WRITE | GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING,
      FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);
  if(tun_fd == INVALID_HANDLE_VALUE) {
    int errcode = GetLastError();

    LOG("failed to open tap device: %d", errcode);
    return INVALID_HANDLE_VALUE;
  }
  LOG("success: tap device opened");

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
  LOG("TunTap::onTunTapData was called");
  // string_view packet = read(tunBuffer);
  // sendToLowerLayer(BadDeviceId, packet);
}

void TunTap::setPowershellStuff(std::string name)
{
  auto identityPath = std::string(getenv("PROGRAMDATA")) + "\\husarnet\\id";
  auto identity = Identity::deserialize(Port::readFile(identityPath));

  std::string sourceNetshName = getNetshNameForGuid(name);
  LOG("sourceNetshName: %s", sourceNetshName.c_str());
  std::string netshName = "Husarnet";
  if(netshName != sourceNetshName) {
    if(mySystem(
           "netsh interface set interface name = \"" + sourceNetshName +
           "\" newname = \"" + netshName + "\"") == 0) {
      LOG("renamed successfully");
    } else {
      netshName = sourceNetshName;
      LOG("rename failed");
    }
  }
  std::string quotedName = "\"" + netshName + "\"";

  mySystem(
      "netsh interface ipv6 add neighbors " + quotedName +
      " fc94:8385:160b:88d1:c2ec:af1b:06ac:0001 52-54-00-fc-94-4d");
  std::string myIp = IpAddress::fromBinary(identity.getDeviceId()).str();
  LOG("myIp is: %s", myIp.c_str());
  mySystem(
      "netsh interface ipv6 add address " + quotedName + " " + myIp + "/128");
  mySystem(
      "netsh interface ipv6 add route "
      "fc94:8385:160b:88d1:c2ec:af1b:06ac:0001/128 " +
      quotedName);
  mySystem(
      "netsh interface ipv6 add route fc94::/16 " + quotedName +
      " fc94:8385:160b:88d1:c2ec:af1b:06ac:0001");

  std::string cmd =
      "powershell New-NetFirewallRule -DisplayName AllowHusarnet -Direction "
      "Inbound -Action Allow -LocalAddress fc94::/16 -RemoteAddress fc94::/16 "
      "-InterfaceAlias \"\"\"" +
      netshName + "\"\"\"";
  mySystem((cmd).c_str());
}

TunTap::TunTap(std::string name, bool isTap)
{
  (void)isTap;
  tap_fd = openTun(name);
  tunBuffer.resize(4096);


  bringUp();

  setPowershellStuff(name);

  selfMacAddr = getMac();

  // TODO ympek
  // At this point, we should call bindCustomFd and plug
  // our function into select() call from OsSocket
  // but there is mismatch in underlying implementation,
  // as win32 impl uses pointers and unix impl uses regular int descriptors
  // so therefore I decided to simply make a new thread here
  GIL::startThread(
      [&] {
        std::string buf;
        buf.resize(4096);

        while(true) {
          string_view packet = read(buf);
          // L2 packet received, unwrap L2 frame and transmit L3 futher
          // LOG("to: %s (exp: %s), from: %s (exp: %s), proto: %s",
          //     encodeHex(packet.substr(0, 6)).c_str(),
          //     encodeHex(peerMacAddr).c_str(), encodeHex(packet.substr(6,
          //     6)).c_str(), encodeHex(selfMacAddr).c_str(),
          //     encodeHex(packet.substr(12, 2)).c_str());

          if(packet.substr(0, 6) == peerMacAddr &&
             packet.substr(6, 6) == selfMacAddr &&
             packet.substr(12, 2) == string_view("\x86\xdd", 2)) {
            auto packetToShow = encodeHex(std::string(packet.substr(14)));
            LOG("PACKET: size: %llu contents: %s", packet.size(),
                packetToShow.c_str());
            // LOG("PACKET: size: %llu", packet.size());
            sendToLowerLayer(BadDeviceId, packet.substr(14));
          } else {
            LOG("WRONG PACKET: size: %llu", packet.size());
          }
        }
      },
      "wintap-read");
  // NgSocket* netDev = L2Unwrapper::wrap(netDevL3, winTap->getMac());

  // netDev->delegate = new WinTapDelegate(winTap);
  // bindControlSocket(configManager);
}

string_view TunTap::read(std::string& buffer)
{
  return GIL::unlocked<string_view>([&] {
    OVERLAPPED overlapped = {0, 0, 0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if(ReadFile(tap_fd, &buffer[0], (DWORD)buffer.size(), NULL, &overlapped) ==
       0) {
      if(GetLastError() != ERROR_IO_PENDING) {
        LOG("tap read failed %d", (int)GetLastError());
        assert(false);
      }
    }

    WaitForSingleObject(overlapped.hEvent, INFINITE);
    CloseHandle(overlapped.hEvent);

    int len = overlapped.InternalHigh;
    return string_view(buffer).substr(0, len);
  });
}

void TunTap::write(string_view data)
{
  GIL::unlocked<void>([&] {
    OVERLAPPED overlapped = {0, 0, 0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(WriteFile(tap_fd, data.data(), (DWORD)data.size(), NULL, &overlapped)) {
      auto error = GetLastError();
      if(error != ERROR_IO_PENDING) {
        LOG("tap write failed");
      }
    }

    WaitForSingleObject(overlapped.hEvent, INFINITE);
    CloseHandle(overlapped.hEvent);
  });
}

void TunTap::bringUp()
{
  DWORD len;

  ULONG flag = 1;
  if(DeviceIoControl(
         tap_fd, TAP_IOCTL_SET_MEDIA_STATUS, &flag, sizeof(flag), &flag,
         sizeof(flag), &len, NULL) == 0) {
    LOG("failed to bring up the tap device");
    return;
  }

  LOG("tap config OK");
}

std::string TunTap::getMac()
{
  std::string hwaddr;
  hwaddr.resize(6);
  DWORD len;

  if(DeviceIoControl(
         tap_fd, TAP_IOCTL_GET_MAC, &hwaddr[0], hwaddr.size(), &hwaddr[0],
         hwaddr.size(), &len, NULL) == 0) {
    LOG("failed to retrieve MAC address");
    assert(false);
  } else {
    LOG("MAC address: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x", 0xFF & hwaddr[0],
        0xFF & hwaddr[1], 0xFF & hwaddr[2], 0xFF & hwaddr[3], 0xFF & hwaddr[4],
        0xFF & hwaddr[5]);
  }
  return hwaddr;
}

void TunTap::onLowerLayerData(DeviceId source, string_view data)
{
  (void)source;
  LOG("TunTap::onLowerLayerData, DeviceId: %s data size is: %lld",
      encodeHex(source).c_str(), data.size());
  // HMM
  // TODO ympek implement
  std::string wrapped;

  wrapped += selfMacAddr;
  wrapped += peerMacAddr;
  wrapped += string_view("\x86\xdd", 2);
  wrapped += data;
  write(wrapped);
  // if(wr != data.size()) {
  //   LOG("short tun write");
  // }
}
