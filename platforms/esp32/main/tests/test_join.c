// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "common.h"
#include <lwip/sockets.h>

const size_t JOIN_TIMEOUT_S = 20;
const size_t HOSTNAME_LEN = 32;
const size_t JOINCODE_LEN = 64;

static const char *TAG = "test";
static HusarnetClient* husarnetClient;

bool wait_for_join(size_t timeout_s) {
  for (int i = 0; i < timeout_s; i++) {
    ESP_LOGI(TAG, "Joining Husarnet network...");

    // Check if the device has successfully joined the network  
    if (husarnet_is_joined(husarnetClient))
      return true;

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  return false;
}

TEST_CASE("join", "[husarnet]")
{
  ESP_LOGI(TAG, "Starting Husarnet client...");

  // Initialize Husarnet client
  husarnetClient = husarnet_init();

  // Get Husarnet hostname and join code from the terminal
  char hostname[HOSTNAME_LEN];
  char join_code[JOINCODE_LEN];

  ESP_LOGI(TAG, "Hostname:");
  unity_gets(hostname, HOSTNAME_LEN);
  ESP_LOGI(TAG, "Join code:");
  esp_read_line_no_echo(join_code, JOINCODE_LEN);

  // This function joins the network and blocks until the connection is established
  husarnet_join(husarnetClient, hostname, join_code);

  UNITY_TEST_ASSERT(
    wait_for_join(JOIN_TIMEOUT_S),
    __LINE__, 
    "Timed out while waiting for the device to join the network");
  
  // Get Husarnet IP address
  char ip[HUSARNET_IP_STR_LEN];
  husarnet_get_ip_address(husarnetClient, ip, HUSARNET_IP_STR_LEN);
  ESP_LOGI(TAG, "Husarnet IP address: %s", ip);

  // Validate the IP address
  struct in6_addr addr;
  UNITY_TEST_ASSERT(inet_pton(AF_INET6, ip, &addr) == 1, __LINE__, "Got malformed IPv6 address");

  // Wait for all websetup transactions to be finished
  // and printed to the console
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  husarnet_disable_extended_logging();
}
