// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <husarnet/husarnet_manager.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void run_husarnet(TaskHandle_t* husarnet_task_handle)
{
  auto* manager = new HusarnetManager();

  xTaskCreate([](void* manager) {
    ((HusarnetManager*)manager)->runHusarnet();
  }, "husarnet_task", 16384 * 8, manager, 5, husarnet_task_handle);
}
