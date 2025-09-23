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

static void CALLBACK WintunLoggerToHusarnetLogger(WINTUN_LOGGER_LEVEL Level, DWORD64 Timestamp, const WCHAR* LogLine)
{
  constexpr size_t bufferSize = 512;
  auto logLineBuffer = static_cast<char*>(malloc(bufferSize));
  wcstombs(logLineBuffer, LogLine, _TRUNCATE);

  SYSTEMTIME SystemTime;
  FileTimeToSystemTime((FILETIME*)&Timestamp, &SystemTime);
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

  // TODO: try to observe first if the memory leaks
  // free(logLineBuffer);
}

TunTap::TunTap(const HusarnetAddress address) : valid(false), husarnetAddress(address)
{
  this->init();
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

bool TunTap::init()
{
  this->wintunLib =
      LoadLibraryExW(L"wintun.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!this->wintunLib) {
    LOG_CRITICAL("TunLayer: wintun.dll not found");
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
    LOG_CRITICAL("TunLayer: failed to initialize wintun library");
    return false;
  }
  // clang-format on

  return true;
}

bool TunTap::start()
{
  this->acquireWintunAdapter();
  if(!this->wintunAdapter) {
    LOG_CRITICAL("TunLayer: failed to open/create wintun adapter")
    return false;
  }

  WintunSetLogger(WintunLoggerToHusarnetLogger);
  DWORD Version = WintunGetRunningDriverVersion();
  LOG_INFO("TunLayer: wintun v%u.%u loaded", (Version >> 16) & 0xff, (Version >> 0) & 0xff);
  this->assignIpAddressToAdapter(this->husarnetAddress);
  this->wintunSession = WintunStartSession(this->wintunAdapter, ringCapacity);
  if(!this->wintunSession) {
    LOG_CRITICAL("TunLayer: unable to start wintun session")
  }
  return true;
}

void TunTap::startReaderThread()
{
  Port::threadStart([this]() {
    HANDLE WaitHandles[] = {WintunGetReadWaitEvent(this->wintunSession)};
    while(true) {
      DWORD packetSize;
      BYTE* Packet = WintunReceivePacket(this->wintunSession, &packetSize);
      if(Packet) {
        this->sendToLowerLayer(IpAddress(), string_view(reinterpret_cast<const char*>(Packet), packetSize));
        WintunReleaseReceivePacket(this->wintunSession, Packet);
      } else {
        DWORD LastError = GetLastError();
        switch(LastError) {
          case ERROR_NO_MORE_ITEMS:
            if(WaitForMultipleObjects(_countof(WaitHandles), WaitHandles, FALSE, INFINITE) == WAIT_OBJECT_0)
              continue; // TODO wait for single object actually
          default:
            LOG_ERROR("TunLayer: packet read failed")
        }
      }
    }
  }, "wintun-reader", 8000, WINTUN_READER_TASK_PRIORITY);
}

void TunTap::acquireWintunAdapter()
{
  this->wintunAdapter = WintunOpenAdapter(networkAdapterNameSz);
  if(!this->wintunAdapter) {
    LOG_INFO("TunLayer: existing wintun adapter not found, creating new one... %d", GetLastError());
    // since GUIDs are 128-bit values, IMO we can simply provide Husarnet IPv6 here
    // according to the Wintun docs GUID parameter is a suggestion anyway
    // byte order will be not exactly what you expect but not important
    // TODO: consider the case when we regenerate ID (does the old GUID stay in the system and clutter somehow?)
    GUID myGuid;
    memcpy(&myGuid, &this->husarnetAddress, 16);
    LOG_INFO("trying the create");
    this->wintunAdapter = WintunCreateAdapter(networkAdapterNameSz, L"WintunHusarnetTodoChange", &myGuid);
    LOG_INFO("something happened %d", GetLastError());
    if(!this->wintunAdapter) {
      LOG_INFO("inside if");
      DWORD errorCode = GetLastError();
      if(errorCode == ERROR_ACCESS_DENIED) {
        LOG_INFO("ACCESS_DENIED while trying to open tunnel, are you Administrator? Error code: %d", errorCode);
      } else if(errorCode == ERROR_ALREADY_EXISTS) {
        LOG_INFO("ALREADY_EXISTS while trying to create wintun adapter");
        // one last try to open it
        this->wintunAdapter = WintunOpenAdapter(networkAdapterNameSz);
        if(!this->wintunAdapter) {
          LOG_INFO("can't open adapter");
        } // TODO: uhhhh
      }
      else {
        LOG_ERROR("Unable to create wintun adapter. Error code : %d", errorCode);
      }
    } else {
      LOG_INFO("wintun adapter created");
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

std::string TunTap::getAdapterName()
{
  return networkAdapterNameStr;
}

bool TunTap::isValid() const
{
  return this->valid;
}
