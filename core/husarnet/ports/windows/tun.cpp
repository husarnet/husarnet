// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/windows/tun.h"

#include <functional>

#include <iphlpapi.h>
#include <netioapi.h>
#include <process.h>
#include <winternl.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

#include "husarnet/logging.h"

#include "dummy_task_priorities.h"
#include "port_interface.h"

static WINTUN_CREATE_ADAPTER_FUNC* WintunCreateAdapter;
static WINTUN_CLOSE_ADAPTER_FUNC* WintunCloseAdapter;
static WINTUN_OPEN_ADAPTER_FUNC* WintunOpenAdapter;
static WINTUN_GET_ADAPTER_LUID_FUNC* WintunGetAdapterLUID;
static WINTUN_GET_RUNNING_DRIVER_VERSION_FUNC* WintunGetRunningDriverVersion;
static WINTUN_DELETE_DRIVER_FUNC* WintunDeleteDriver;
static WINTUN_SET_LOGGER_FUNC* WintunSetLogger;
static WINTUN_START_SESSION_FUNC* WintunStartSession;
static WINTUN_END_SESSION_FUNC* WintunEndSession;
static WINTUN_GET_READ_WAIT_EVENT_FUNC* WintunGetReadWaitEvent;
static WINTUN_RECEIVE_PACKET_FUNC* WintunReceivePacket;
static WINTUN_RELEASE_RECEIVE_PACKET_FUNC* WintunReleaseReceivePacket;
static WINTUN_ALLOCATE_SEND_PACKET_FUNC* WintunAllocateSendPacket;
static WINTUN_SEND_PACKET_FUNC* WintunSendPacket;

static void CALLBACK WintunLoggerToHusarnetLogger(WINTUN_LOGGER_LEVEL Level, DWORD64 Timestamp, const WCHAR* LogLine)
{
  constexpr size_t bufferSize = 512;
  auto logLineBuffer = static_cast<char*>(malloc(bufferSize));
  wcstombs(logLineBuffer, LogLine, _TRUNCATE);
  (void)Timestamp;

  switch(Level) {
    case WINTUN_LOG_WARN:
      HLOG_WARNING("wintun warning // {message}", logLineBuffer);
      break;
    case WINTUN_LOG_ERR:
      HLOG_ERROR("wintun error // {message}", logLineBuffer);
      break;
    default:
      HLOG_DEBUG("wintun message // {message}", logLineBuffer);
      break;
  }

  free(logLineBuffer);
}

Tun::Tun(const HusarnetAddress address) : husarnetAddress(address)
{
  this->init();
}

Tun::~Tun()
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

bool Tun::init()
{
  this->wintunLib =
      LoadLibraryExW(L"wintun.dll", NULL, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
  if(!this->wintunLib) {
    HLOG_CRITICAL("TunLayer: wintun.dll not found");
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
    HLOG_CRITICAL("TunLayer: failed to initialize wintun library");
    return false;
  }
  // clang-format on

  return true;
}

bool Tun::start()
{
  this->acquireWintunAdapter();
  if(!this->wintunAdapter) {
    HLOG_CRITICAL("TunLayer: failed to open/create wintun adapter");
    return false;
  }

  WintunSetLogger(WintunLoggerToHusarnetLogger);
  DWORD Version = WintunGetRunningDriverVersion();
  std::string wintunVersion = std::to_string((Version >> 16) & 0xff) + "." + std::to_string((Version >> 0) & 0xff);
  HLOG_INFO("TunLayer: wintun loaded // {wintun_version}", wintunVersion);
  this->assignIpAddressToAdapter(this->husarnetAddress);
  this->wintunSession = WintunStartSession(this->wintunAdapter, ringCapacity);
  if(!this->wintunSession) {
    HLOG_CRITICAL("TunLayer: unable to start wintun session");
    return false;
  }
  return true;
}

void Tun::startReaderThread()
{
  Port::threadStart(
      [this]() {
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
                  continue;  // TODO wait for single object actually
              default:
                HLOG_ERROR("TunLayer: packet read failed");
            }
          }
        }
      },
      "wintun-reader", 8000, WINTUN_READER_TASK_PRIORITY);
}

void Tun::acquireWintunAdapter()
{
  this->wintunAdapter = WintunOpenAdapter(networkAdapterNameSz);
  if(!this->wintunAdapter) {
    HLOG_INFO("TunLayer: existing wintun adapter not found, creating new one // {errno}", GetLastError());
    // since GUIDs are 128-bit values, IMO we can simply provide Husarnet IPv6 here
    // according to the Wintun docs GUID parameter is a suggestion anyway
    // byte order will be not exactly what you expect but not important
    // TODO: consider the case when we regenerate ID (does the old GUID stay in the system and clutter somehow?)
    GUID myGuid;
    memcpy(&myGuid, &this->husarnetAddress, 16);
    this->wintunAdapter = WintunCreateAdapter(networkAdapterNameSz, L"WintunHusarnetTodoChange", &myGuid);
    if(!this->wintunAdapter) {
      DWORD errorCode = GetLastError();
      if(errorCode == ERROR_ACCESS_DENIED) {
        HLOG_ERROR("ACCESS_DENIED while trying to open tunnel, are you Administrator? // {errno}", errorCode);
      } else if(errorCode == ERROR_ALREADY_EXISTS) {
        HLOG_ERROR("ALREADY_EXISTS while trying to create wintun adapter");
        // one last try to open it
        this->wintunAdapter = WintunOpenAdapter(networkAdapterNameSz);
        if(!this->wintunAdapter) {
          HLOG_INFO("can't open adapter");
        }  // TODO: figure out why this sometimes happens (wintun does not teardown cleanly after killing process?)
      } else {
        HLOG_ERROR("unable to create wintun adapter. // {error_code}", errorCode);
      }
    } else {
      HLOG_INFO("wintun adapter created");
    }
  } else {
    HLOG_INFO("wintun adapter opened");
  }
}

void Tun::assignIpAddressToAdapter(HusarnetAddress addr)
{
  MIB_UNICASTIPADDRESS_ROW AddressRow;
  InitializeUnicastIpAddressEntry(&AddressRow);
  WintunGetAdapterLUID(this->wintunAdapter, &AddressRow.InterfaceLuid);
  AddressRow.Address.Ipv6.sin6_family = AF_INET6;
  memcpy(AddressRow.Address.Ipv6.sin6_addr.s6_addr, addr.data.data(), 16);
  AddressRow.OnLinkPrefixLength = 32;
  AddressRow.DadState = IpDadStatePreferred;
  auto LastError = CreateUnicastIpAddressEntry(&AddressRow);
  if(LastError != ERROR_SUCCESS && LastError != ERROR_OBJECT_ALREADY_EXISTS) {
    HLOG_ERROR("TunLayer: cannot assign IP addr to interface // {errno}", LastError);
  }
}

void Tun::closeWintunAdapter()
{
  WintunCloseAdapter(this->wintunAdapter);
}

void Tun::onLowerLayerData(HusarnetAddress source, string_view data)
{
  BYTE* Packet = WintunAllocateSendPacket(this->wintunSession, data.size());
  if(Packet) {
    memcpy(Packet, data.data(), data.size());
    WintunSendPacket(this->wintunSession, Packet);
  } else if(GetLastError() == ERROR_BUFFER_OVERFLOW) {
    HLOG_ERROR("packet write failed - buffer overflow");
  } else {
    HLOG_ERROR("packet write failed // {errno}", GetLastError());
  }
}

std::string Tun::getAdapterName()
{
  return networkAdapterNameStr;
}
