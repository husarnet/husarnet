// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ports/windows/wintun.h"
#include <windows.h>

#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"


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

// Adapter name will show up in Control panel -> Network Connections, in Wireshark, etc...
constexpr LPCWSTR networkAdapterName = L"HusarnetNewImplementation";
// Rings capacity. As per Wintun docs: must be between WINTUN_MIN_RING_CAPACITY and WINTUN_MAX_RING_CAPACITY (incl.)
// Must be a power of two.
constexpr int ringCapacity = 0x400000;

class TunTap : public UpperLayer {
public:
  TunTap(HusarnetAddress address);
  TunTap(const TunTap&) = delete;
  TunTap(TunTap&&) = delete;
  TunTap& operator=(const TunTap&) = delete;
  TunTap& operator=(TunTap&&) = delete;
  ~TunTap();
  bool init();
  bool start(HusarnetAddress);
  bool wintunSend(BYTE* data, DWORD length);

private:
  void acquireWintunAdapter();
  void assignIpAddressToAdapter(HusarnetAddress addr);
  void closeWintunAdapter();
  void onLowerLayerData(HusarnetAddress source, string_view data) override;

  bool isValid;
  HMODULE wintunLib;
  WINTUN_ADAPTER_HANDLE wintunAdapter;
  WINTUN_SESSION_HANDLE wintunSession;
  HusarnetAddress husarnetAddress;
};
