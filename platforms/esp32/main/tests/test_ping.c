#include "common.h"
#include "ping/ping_sock.h"
#include "lwip/sockets.h"

static uint32_t min_rtt = UINT32_MAX;
static uint32_t max_rtt = 0;
static uint32_t rtt_sum = 0;
static bool ping_finished = false;

static const char *TAG = "test";

inline static const char* inet_generic_ntop(ip_addr_t ip) {
  static char buf[IPADDR_STRLEN_MAX];

  if (ip.type == IPADDR_TYPE_V6) {
    inet_ntop(AF_INET6, &ip.u_addr.ip6, buf, IPADDR_STRLEN_MAX);
  } else if (ip.type == IPADDR_TYPE_V4) {
    inet_ntop(AF_INET, &ip.u_addr.ip4, buf, IPADDR_STRLEN_MAX);
  } else {
    ESP_LOGE(TAG, "Invalid IP address");
    TEST_FAIL();
  }

  return buf;
}

static void test_on_ping_success(esp_ping_handle_t hdl, void *args) {
  uint8_t ttl;
  uint16_t seqno;
  uint32_t elapsed_time, recv_len;
  ip_addr_t target_addr;

  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
  esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
  esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
  esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

  // Calculate RTT stats
  if (elapsed_time > max_rtt)
    max_rtt = elapsed_time;

  if (elapsed_time < min_rtt)
    min_rtt = elapsed_time;

  rtt_sum += elapsed_time;

  ESP_LOGI(TAG, "%lu bytes from %s icmp_seq=%hu ttl=%hhu time=%lu ms",
          recv_len, inet_generic_ntop(target_addr), seqno, ttl, elapsed_time);
}

static void test_on_ping_timeout(esp_ping_handle_t hdl, void *args) {
  uint16_t seqno;
  ip_addr_t target_addr;

  esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
  esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));

  ESP_LOGI(TAG, "From %s icmp_seq=%d timeout", inet_generic_ntop(target_addr), seqno);
}

static void test_on_ping_end(esp_ping_handle_t hdl, void *args) {
  ping_finished = true;
}

TEST_CASE("ping", "[husarnet]")
{
  husarnet_disable_extended_logging();

  esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
  
  // Reset RTT stats
  max_rtt = 0;
  min_rtt = UINT32_MAX;
  rtt_sum = 0;

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

  ping_config.target_addr = target_ip;

  // Get transmitted data size from the terminal
  ESP_LOGI(TAG, "Data size:");
  unity_gets(target_ip_str, IPADDR_STRLEN_MAX);
  int data_size = atoi(target_ip_str);

  if (data_size < 0 || data_size > 1500) {
    ESP_LOGE(TAG, "Invalid data size");
    TEST_FAIL();
  }

  ping_config.data_size = data_size;

  // Get duration from the terminal
  ESP_LOGI(TAG, "Duration:");
  unity_gets(target_ip_str, IPADDR_STRLEN_MAX);
  int duration = atoi(target_ip_str);

  if (duration < 0) {
    ESP_LOGE(TAG, "Invalid duration");
    TEST_FAIL();
  }

  ping_config.count = duration;

  // Set the callback functions
  esp_ping_callbacks_t cbs = {
    .on_ping_success = test_on_ping_success,
    .on_ping_timeout = test_on_ping_timeout,
    .on_ping_end = test_on_ping_end,
    .cb_args = NULL,
  };

  // Create a new ping session
  esp_ping_handle_t ping;
  TEST_ESP_OK(esp_ping_new_session(&ping_config, &cbs, &ping));

  // Start the ping session
  husarnet_enable_extended_logging();
  ping_finished = false;
  TEST_ESP_OK(esp_ping_start(ping));

  // Wait for the ping session to finish
  while (!ping_finished) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  // Print ping statistics
  uint32_t transmitted;
  uint32_t received;
  uint32_t total_time_ms;

  esp_ping_get_profile(ping, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
  esp_ping_get_profile(ping, ESP_PING_PROF_REPLY, &received, sizeof(received));
  esp_ping_get_profile(ping, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
  float avg_rtt = (float)rtt_sum / received;

  ESP_LOGI(TAG, "%lu packets transmitted, %lu received, time %lu ms", transmitted, received, total_time_ms);
  ESP_LOGI(TAG, "rtt min/avg/max = %lu/%.2f/%lu ms", min_rtt, avg_rtt, max_rtt);

  // Clean up
  esp_ping_delete_session(ping);

  // Verify the test results
  TEST_ASSERT_EQUAL(transmitted, ping_config.count);
  TEST_ASSERT_GREATER_OR_EQUAL(ping_config.count * 0.8, received);
  TEST_ASSERT_LESS_OR_EQUAL(500, avg_rtt);

  husarnet_disable_extended_logging();

  // Add a delay to prevent pexpect from clearing the buffer too early
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
