#include "iperf_wrapper.h"
#include "iperf.h"
#include "esp_check.h"
#include "esp_log.h"
#include "lwip/sockets.h"

static const char *TAG = "main";

void start_iperf_client(struct ip_addr* addr, uint32_t duration) {
  iperf_cfg_t cfg = {0};

  cfg.flag |= IPERF_FLAG_CLIENT;

  // Set the source and destination IPs
  cfg.source_ip4 = INADDR_ANY;
  cfg.source_ip6 = "::"; // Currently ignored in the ESP iperf library
  
  if (addr->type == IPADDR_TYPE_V4) {
    cfg.type = IPERF_IP_TYPE_IPV4;
    cfg.destination_ip4 = addr->u_addr.ip4.addr;
  } else if (addr->type == IPADDR_TYPE_V6) {
    cfg.type = IPERF_IP_TYPE_IPV6;
    cfg.destination_ip6 = inet6_ntoa(addr->u_addr.ip6);
  } else {
    ESP_LOGE(TAG, "Invalid address family");
    return;
  }

  // Set TCP as the transport protocol for now
  cfg.flag |= IPERF_FLAG_TCP;

  // Set default port numbers
  cfg.sport = IPERF_DEFAULT_PORT;
  cfg.dport = IPERF_DEFAULT_PORT;
  
  cfg.time = duration;
  cfg.interval = IPERF_DEFAULT_INTERVAL;
  cfg.bw_lim = IPERF_DEFAULT_NO_BW_LIMIT;

  iperf_start(&cfg);
}

void start_iperf_server(uint32_t duration) {
  iperf_cfg_t cfg = {0};

  cfg.flag |= IPERF_FLAG_SERVER;

  // Set the source and destination IPs
  cfg.source_ip4 = INADDR_ANY;
  cfg.source_ip6 = "::"; // Currently ignored in the ESP iperf library

  // Set TCP as the transport protocol for now
  cfg.flag |= IPERF_FLAG_TCP;

  // Set default port numbers
  cfg.sport = IPERF_DEFAULT_PORT;
  cfg.dport = IPERF_DEFAULT_PORT;

  cfg.time = duration;
  cfg.interval = IPERF_DEFAULT_INTERVAL;
  cfg.bw_lim = IPERF_DEFAULT_NO_BW_LIMIT;

  iperf_start(&cfg);
}

void stop_iperf() {
  iperf_stop();
}
