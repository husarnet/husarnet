// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/windows/tun.h"

#include <functional>
#include <iostream>

#include <iphlpapi.h>
#include <netioapi.h>
#include <process.h>
#include <winternl.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

#include "husarnet/logging.h"

#include "dummy_task_priorities.h"
#include "port_interface.h"

// we need to convert wchar

static void CALLBACK WintunLoggerToHusarnetLogger(WINTUN_LOGGER_LEVEL Level, DWORD64 Timestamp, const WCHAR* LogLine)
{
  constexpr size_t bufferSize = 512;
  auto logLineBuffer = static_cast<char*>(malloc(bufferSize));
  auto writtenBytes = wcstombs(logLineBuffer, LogLine, _TRUNCATE);

  SYSTEMTIME SystemTime;
  FileTimeToSystemTime((FILETIME*)&Timestamp, &SystemTime);
  WCHAR LevelMarker;
  switch(Level) {
    case WINTUN_LOG_WARN:
      LOG_WARNING("wintun: %04u-%02u-%02u %02u:%02u:%02u.%04u %s", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
      SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds, logLineBuffer)
      break;
    case WINTUN_LOG_ERR:
      LOG_ERROR("wintun: %04u-%02u-%02u %02u:%02u:%02u.%04u %s", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
      SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds, logLineBuffer)
      break;
    default:
      LOG_DEBUG("wintun: %04u-%02u-%02u %02u:%02u:%02u.%04u %s", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds, logLineBuffer)
      break;
  }
}

static DWORD64 Now()
{
  LARGE_INTEGER ret;
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  ret.LowPart = ft.dwLowDateTime;
  ret.HighPart = ft.dwHighDateTime;
  return ret.QuadPart;
}

static USHORT Checksum(const BYTE* buf, int len)
{
  unsigned long sum = 0;
  while(len > 1) {
    sum += *(USHORT*)buf;
    buf += 2;
    len -= 2;
  }
  if(len)
    sum += *(BYTE*)buf << 8;

  while(sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);

  return ~((USHORT)sum);
}

static void MakeICMPv6(BYTE Packet[48])
{
  memset(Packet, 0, 48);

  // IPv6 Header (40 bytes)
  Packet[0] = 0x60;  // Version = 6
  Packet[6] = 58;    // Next Header = 58 (ICMPv6)
  Packet[7] = 255;   // Hop Limit

  // Payload Length = 8 bytes (ICMPv6 Echo Request)
  *(USHORT*)&Packet[4] = htons(8);

  // Source Address: 2001:db8::1
  IN6_ADDR src = IN6ADDR_LOOPBACK_INIT;
  inet_pton(AF_INET6, "fc94:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa", &src);
  memcpy(&Packet[8], &src, 16);

  // Destination Address: 2001:db8::2
  IN6_ADDR dst;
  inet_pton(AF_INET6, "fc94:aaaa:aaaa:aaaa:aaaa:aaaa:aaaa:bbbb", &dst);
  memcpy(&Packet[24], &dst, 16);

  // ICMPv6 Echo Request starts at byte 40
  Packet[40] = 128;  // Type = 128 (Echo Request)
  Packet[41] = 0;    // Code = 0
  Packet[42] = 0;    // Checksum placeholder
  Packet[43] = 0;
  Packet[44] = 0x12;  // Identifier (arbitrary)
  Packet[45] = 0x34;
  Packet[46] = 0x00;  // Sequence Number
  Packet[47] = 0x01;

  // Compute ICMPv6 checksum (requires pseudo-header)
  BYTE pseudoHeader[40 + 8 + 8] = {0};
  memcpy(&pseudoHeader[0], &Packet[8], 16);    // Source address
  memcpy(&pseudoHeader[16], &Packet[24], 16);  // Destination address
  *(ULONG*)&pseudoHeader[32] = htonl(8);       // Payload length
  pseudoHeader[39] = 58;                       // Next header

  memcpy(&pseudoHeader[40], &Packet[40], 8);  // ICMPv6 packet

  USHORT cksum = Checksum(pseudoHeader, 48);
  *(USHORT*)&Packet[42] = cksum;

  LOG_INFO("Sending IPv6 ICMP echo request to 2001:db8::2 from 2001:db8::1");
}

TunTap::TunTap(HusarnetAddress addr) : husarnetAddress(addr)
{
  this->init();
  this->start(addr);
}

TunTap::~TunTap()
{
  if(this->wintunSession) {
    WintunEndSession(this->wintunSession);
  }
  if(this->wintunAdapter) {
    WintunCloseAdapter(this->wintunAdapter);
  }
  if(this->wintunLib) {
    FreeLibrary(this->wintunLib);
  }
}

// checks the library exists and initializes it
bool TunTap::init()
{
  this->wintunLib =
      LoadLibraryExW(L"wintun.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!this->wintunLib) {
    LOG_INFO("critical: WintunManager: wintun.dll not found");
    return false;
  }

  // clang-format off
  if(((*(FARPROC*)&WintunCreateAdapter =           GetProcAddress(this->wintunLib, "WintunCreateAdapter")) == NULL) ||
     ((*(FARPROC*)&WintunCloseAdapter =            GetProcAddress(this->wintunLib, "WintunCloseAdapter")) == NULL) ||
     ((*(FARPROC*)&WintunOpenAdapter =             GetProcAddress(this->wintunLib, "WintunOpenAdapter")) == NULL) ||
     ((*(FARPROC*)&WintunGetAdapterLUID =          GetProcAddress(this->wintunLib, "WintunGetAdapterLUID")) == NULL) ||
     ((*(FARPROC*)&WintunGetRunningDriverVersion = GetProcAddress(this->wintunLib, "WintunGetRunningDriverVersion")) == NULL) ||
     ((*(FARPROC*)&WintunDeleteDriver =            GetProcAddress(this->wintunLib, "WintunDeleteDriver")) == NULL) ||
     ((*(FARPROC*)&WintunSetLogger =               GetProcAddress(this->wintunLib, "WintunSetLogger")) == NULL) ||
     ((*(FARPROC*)&WintunStartSession =            GetProcAddress(this->wintunLib, "WintunStartSession")) == NULL) ||
     ((*(FARPROC*)&WintunEndSession =              GetProcAddress(this->wintunLib, "WintunEndSession")) == NULL) ||
     ((*(FARPROC*)&WintunGetReadWaitEvent =        GetProcAddress(this->wintunLib, "WintunGetReadWaitEvent")) == NULL) ||
     ((*(FARPROC*)&WintunReceivePacket =           GetProcAddress(this->wintunLib, "WintunReceivePacket")) == NULL) ||
     ((*(FARPROC*)&WintunReleaseReceivePacket =    GetProcAddress(this->wintunLib, "WintunReleaseReceivePacket")) == NULL) ||
     ((*(FARPROC*)&WintunAllocateSendPacket =      GetProcAddress(this->wintunLib, "WintunAllocateSendPacket")) == NULL) ||
     ((*(FARPROC*)&WintunSendPacket =              GetProcAddress(this->wintunLib, "WintunSendPacket")) == NULL))
  {
    DWORD LastError = GetLastError();
    FreeLibrary(this->wintunLib);
    SetLastError(LastError);
    LOG_INFO("critical: WintunManager: failed to initialize Wintun library");
    return false;
  }
  // clang-format on

  return true;
}

// static HANDLE QuitEvent;

// static BOOL WINAPI CtrlHandler(DWORD CtrlType)
// {
//   switch(CtrlType) {
//     case CTRL_C_EVENT:
//     case CTRL_BREAK_EVENT:
//     case CTRL_CLOSE_EVENT:
//     case CTRL_LOGOFF_EVENT:
//     case CTRL_SHUTDOWN_EVENT:
//       LOG_INFO("Cleaning up and shutting down...");
//       HaveQuit = TRUE;
//       SetEvent(QuitEvent);
//       return TRUE;
//   }
//   return FALSE;
// }

bool TunTap::start(HusarnetAddress addr)
{
  this->acquireWintunAdapter();
  if(!this->wintunAdapter) {
    LOG_INFO("critical: WintunManager: failed to open wintun adapter")
    return false;
  }

  // QuitEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
  // if(!QuitEvent) {
    // LOG_ERROR("CANT CREATE EVENT W");
  // }
  // if(!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
  //   LOG_ERROR("CANT CONSOLE CTRL HANDLER");
  // }

  WintunSetLogger(WintunLoggerToHusarnetLogger);  // TODO: change to our logger (structured logger probably too, later)
  LOG_INFO("WintunManager: wintun library loadded")

  // TODO: cleaning up after (destructor etc
  // TODO: this weird signalling thing

  DWORD Version = WintunGetRunningDriverVersion();
  LOG_INFO("Wintun v%u.%u loaded", (Version >> 16) & 0xff, (Version >> 0) & 0xff);
  this->husarnetAddress = addr;
  this->assignIpAddressToAdapter(this->husarnetAddress);
  this->wintunSession = WintunStartSession(this->wintunAdapter, ringCapacity);
  if(!this->wintunSession) {
    LOG_INFO("WintunStartSession failed - TODO cleanup")
  }

  LOG_INFO("Launching threads and mangling packets...");

  Port::threadStart([this]() {
    HANDLE WaitHandles[] = {WintunGetReadWaitEvent(this->wintunSession)};
    while(true) {
      DWORD PacketSize;
      BYTE* Packet = WintunReceivePacket(this->wintunSession, &PacketSize);
      if(Packet) {
        this->sendToLowerLayer(IpAddress(), string_view(reinterpret_cast<const char*>(Packet), PacketSize));
        WintunReleaseReceivePacket(this->wintunSession, Packet); // TODO not sure if we do this
      } else {
        DWORD LastError = GetLastError();
        switch(LastError) {
          case ERROR_NO_MORE_ITEMS:
            if(WaitForMultipleObjects(_countof(WaitHandles), WaitHandles, FALSE, INFINITE) == WAIT_OBJECT_0)
              continue;
          default:
            LOG_ERROR("packet read failed")
        }
      }
    }
  }, "wintun-reader", 8000, WINTUN_READER_TASK_PRIORITY);

  return true;
}

void TunTap::acquireWintunAdapter()
{
  this->wintunAdapter = WintunOpenAdapter(networkAdapterName);
  if(!this->wintunAdapter) {
    LOG_INFO("Existing wintun adapter not found, creating new one... %d", GetLastError());
    // since GUIDs are 128-bit values, IMO we can simply provide Husarnet IPv6 here
    // according to the Wintun docs GUID parameter it's a suggestion anyway
    // byte order will be not exactly what you expect but not important
    // TODO: consider the case when we regenerate ID (does the old GUID stay in the system and clutter somehow?)
    GUID myGuid;
    memcpy(&myGuid, &this->husarnetAddress, 16);

    this->wintunAdapter = WintunCreateAdapter(networkAdapterName, L"WintunHusarnetTodoChange", &myGuid);
    if(!this->wintunAdapter) {
      DWORD errorCode = GetLastError();
      if(errorCode == ERROR_ACCESS_DENIED) {
        LOG_INFO("ACCESS_DENIED while trying to open tunnel, are you Administrator? Error code: %d", GetLastError());
      } else {
        LOG_ERROR("Unable to create wintun adapter. Error code : %d", errorCode);
      }
      this->isValid = false;  // TODO: utilize it
    }
  } else {
    LOG_INFO("Opened Wintun adapter!")
  }
}

void TunTap::assignIpAddressToAdapter(HusarnetAddress addr)
{
  MIB_UNICASTIPADDRESS_ROW AddressRow;
  InitializeUnicastIpAddressEntry(&AddressRow);
  WintunGetAdapterLUID(this->wintunAdapter, &AddressRow.InterfaceLuid);
  AddressRow.Address.Ipv6.sin6_family = AF_INET6;
  memcpy(AddressRow.Address.Ipv6.sin6_addr.s6_addr, addr.data.data(), sizeof(addr));
  AddressRow.OnLinkPrefixLength = 32;
  AddressRow.DadState = IpDadStatePreferred;  // TODO: nie wiem co to
  auto LastError = CreateUnicastIpAddressEntry(&AddressRow);
  if(LastError != ERROR_SUCCESS && LastError != ERROR_OBJECT_ALREADY_EXISTS) {
    std::cout << "WintunManager: cannot assign IP addr to interface" << std::endl;
  }
}

void TunTap::closeWintunAdapter()
{
  WintunCloseAdapter(this->wintunAdapter);
}

void TunTap::onLowerLayerData(HusarnetAddress source, string_view data)
{
  LOG_INFO("WINTUN WRITE %d", data.size());
  BYTE* Packet = WintunAllocateSendPacket(this->wintunSession, data.size());
  if(Packet) {
    memcpy(Packet, data.data(), data.size());
    WintunSendPacket(this->wintunSession, Packet);
  } else if(GetLastError() != ERROR_BUFFER_OVERFLOW) {
    LOG_ERROR("packet write failed - buffer overflow")
  } else {
    LOG_ERROR("packet write failed %d", GetLastError())
  }

}
