// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_mac.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_wifi.h"

#include "husarnet.h"

static const char *TAG = "main";
TaskHandle_t husarnet_task_handle = NULL;

void start_webserver();

void memory_watch_task(void *pvParameters) {
    //char task_list[2048];

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Free heap: %.2fKB, lowest %.2fKB", esp_get_free_heap_size()/1024.f, esp_get_minimum_free_heap_size()/1024.f);
        
        if (husarnet_task_handle != NULL)
            ESP_LOGI(TAG, "Free stack: %.2fKB", uxTaskGetStackHighWaterMark(husarnet_task_handle)/1024.f);

        // Display FreeRTOS task list
        // vTaskList(task_list);
        // ESP_LOGD(TAG, "Task list:\n%s", task_list);

        // vTaskGetRunTimeStats(task_list);
        // ESP_LOGD(TAG, "Task runtime stats:\n%s", task_list);
    }
}

void app_main(void) {
    xTaskCreate(memory_watch_task, "memory_watch_task", 6000, NULL, 1, NULL);
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Initialize WiFi or Ethernet, depending on the menuconfig configuration.
    ESP_ERROR_CHECK(example_connect());
    // In order to reduce ping, power saving mode is disabled
    esp_wifi_set_ps(WIFI_PS_NONE);

    ESP_LOGI(TAG, "Starting Husarnet client...");

    // Initialize Husarnet client
    HusarnetClient* client = husarnet_init();
    husarnet_task_handle = xTaskGetHandle("husarnet_task");

    // Join Husarnet network with the given hostname and join code
    husarnet_join(client, "husarnet-esp32-test", "XXXXXXXXXXXXXXXXXXXXXX");
    while (!husarnet_is_joined(client)) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Joining Husarnet network...");
    }

    ESP_LOGI(TAG, "Husarnet client started");

    // Get Husarnet IP address
    char ip[HUSARNET_IP_STR_LEN];
    husarnet_get_ip_address(client, ip, HUSARNET_IP_STR_LEN);
    ESP_LOGI(TAG, "IP address: %s", ip);

    // Start simple webserver on husarnet_ip:80
    start_webserver();

    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
