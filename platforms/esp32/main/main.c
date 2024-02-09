// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_mac.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "wifi.h"

#include "user_interface.h"

static const char *TAG = "main";
TaskHandle_t husarnet_task_handle = NULL;

void memory_watch_task(void *pvParameters) {
    char task_list[2048];

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Free heap: %.2fKB, lowest %.2fKB", esp_get_free_heap_size()/1024.f, esp_get_minimum_free_heap_size()/1024.f);
        ESP_LOGI(TAG, "Free stack: %.2fKB", uxTaskGetStackHighWaterMark(husarnet_task_handle)/1024.f);

        // Display FreeRTOS task list
        // vTaskList(task_list);
        // ESP_LOGD(TAG, "Task list:\n%s", task_list);

        vTaskGetRunTimeStats(task_list);
        ESP_LOGD(TAG, "Task runtime stats:\n%s", task_list);
    }
}

void app_main(void) {
    xTaskCreate(memory_watch_task, "memory_watch_task", 6000, NULL, 1, NULL);
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_init();
    esp_wifi_set_ps(WIFI_PS_NONE);

    ESP_LOGI(TAG, "Starting Husarnet client...");

    HusarnetClient* client = husarnet_init();
    husarnet_start(client);
    husarnet_task_handle = xTaskGetHandle("husarnet_task");

    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
