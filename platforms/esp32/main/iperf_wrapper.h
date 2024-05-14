#pragma once

#include "lwip/sockets.h"

void start_iperf_client(struct ip_addr* addr, uint32_t duration);
void start_iperf_server(uint32_t duration);
void stop_iperf();
