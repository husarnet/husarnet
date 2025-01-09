// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "common.h"
#include "iperf_wrapper.h"

static const uint32_t duration_s = 10;

static const char *TAG = "test";

TEST_CASE("iperf_client", "[husarnet]")
{
  husarnet_disable_extended_logging();

  // Get the target IP address from the terminal
  ip_addr_t target_ip;
  char target_ip_str[IPADDR_STRLEN_MAX];

  ESP_LOGI(TAG, "Target IP address:");
  unity_gets(target_ip_str, IPADDR_STRLEN_MAX);

  if (inet_pton(AF_INET6, target_ip_str, &target_ip.u_addr.ip6)) {
    target_ip.type = IPADDR_TYPE_V6;
  } else if (inet_pton(AF_INET, target_ip_str, &target_ip.u_addr.ip4)) {
    target_ip.type = IPADDR_TYPE_V4;
  } else {
    ESP_LOGE(TAG, "Invalid IP address");
    TEST_FAIL();
  }

  // Wait for host to start the server
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Start the iperf client
  start_iperf_client(&target_ip, duration_s);

  // Wait for the client to finish benchmarking with 3s of extra delay
  vTaskDelay((duration_s + 3) * 1000  / portTICK_PERIOD_MS);
}

TEST_CASE("iperf_server", "[husarnet]")
{
  husarnet_disable_extended_logging();
  
  start_iperf_server(duration_s);

  // Wait for the server to finish
  // Unfortunately, the iperf library hides many details
  // and we cannot check if the server has finished easily.
  // 10 seconds of extra delay should be enough
  vTaskDelay((duration_s + 10) * 1000  / portTICK_PERIOD_MS);

  // Test should be done by now, stop the server
  stop_iperf();
}
