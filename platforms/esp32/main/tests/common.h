// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
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
