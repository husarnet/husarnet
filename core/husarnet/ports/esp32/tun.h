// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/device_id.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/ngsocket.h"
#include "husarnet/string_view.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lwip/ip6_addr.h"
#include "lwip/raw.h"

class TunTap : public UpperLayer {
 private:
  void close();
  ip6_addr_t ipAddr;
  struct netconn* conn;

 public:
  QueueHandle_t tunTapMsgQueue;

  TunTap(ip6_addr_t ip, size_t queueSize);
  void processQueuedPackets();
  void onLowerLayerData(DeviceId source, string_view data) override;
  ip6_addr_t getIp6Addr();
};
