#pragma once

#include "unity.h"
#include "test_utils.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "husarnet.h"

void esp_read_line_no_echo(char* dst, size_t len);
void husarnet_enable_extended_logging();
void husarnet_disable_extended_logging();
