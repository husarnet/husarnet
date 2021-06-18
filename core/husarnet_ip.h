#ifndef HUSARNET_IP_
#define HUSARNET_IP_
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    #define HUSARNET_SOCK_STREAM 1
    #define HUSARNET_SOCK_DGRAM 2
    #define HUSARNET_AF_INET6 10

    int husarnet_socket(int domain, int type, int protocol);
    int husarnet_bind(int sockfd, const struct husarnet_sockaddr* addr, socklen_t addrlk);

#ifdef __cplusplus
}
#endif

#endif HUSARNET_IP_
