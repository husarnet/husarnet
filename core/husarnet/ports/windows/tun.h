// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <windows.h>

#include "husarnet/ports/windows/wintun.h"

#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"

// Adapter name will show up in Control panel -> Network Connections, in Wireshark, etc...
// Keeping both widechar and regular version of the string as constexpr
// This might look ugly but saves me a lot of conversion headache
#define NETWORK_ADAPTER_NAME "HusarnetNewImplementation"
constexpr LPCWSTR networkAdapterNameSz = L"" NETWORK_ADAPTER_NAME;
constexpr char networkAdapterNameStr[] = NETWORK_ADAPTER_NAME;
// Rings capacity. As per Wintun docs: must be between WINTUN_MIN_RING_CAPACITY and WINTUN_MAX_RING_CAPACITY (incl.)
// Must be a power of two.
constexpr int ringCapacity = 0x400000;

class Tun : public UpperLayer {
 public:
  Tun(HusarnetAddress address);
  Tun(const Tun&) = delete;
  Tun(Tun&&) = delete;
  Tun& operator=(const Tun&) = delete;
  Tun& operator=(Tun&&) = delete;
  ~Tun();
  bool init();
  bool start();
  void startReaderThread();  // this is separate from interface bringup to reduce noise
  std::string getAdapterName();

 private:
  void acquireWintunAdapter();
  void assignIpAddressToAdapter(HusarnetAddress addr);
  void closeWintunAdapter();
  void onLowerLayerData(HusarnetAddress source, string_view data) override;

  HMODULE wintunLib{NULL};
  WINTUN_ADAPTER_HANDLE wintunAdapter{NULL};
  WINTUN_SESSION_HANDLE wintunSession{NULL};
  HusarnetAddress husarnetAddress;
};
