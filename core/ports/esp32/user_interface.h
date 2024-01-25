#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

void run_husarnet(TaskHandle_t* husarnet_task_handle);

#ifdef __cplusplus
}
#endif