// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "husarnet/device_id.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/ngsocket.h"
#include "husarnet/string_view.h"

class TunTap : public UpperLayer {
 private:
  void close();

 public:
  QueueHandle_t tunTapMsgQueue;

  TunTap(size_t queueSize);
  void processQueuedPackets();
  void onLowerLayerData(DeviceId source, string_view data) override;
};
