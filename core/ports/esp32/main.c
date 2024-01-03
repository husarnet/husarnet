// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// This is a dummy file, just to make linker happy

#include "esp_check.h"
#include "nvs_flash.h"

#include "user_interface.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI(TAG, "Starting Husarnet client...");

    run_husarnet();
}
