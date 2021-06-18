#ifndef LWIP_LWIPOPTS_H
#define LWIP_LWIPOPTS_H

#ifdef ESP_PLATFORM
#error "including wrong file!"
#endif

#ifdef __ANDROID__
#ifdef LWIP_PLATFORM_ASSERT
#undef LWIP_PLATFORM_ASSERT
#endif

// on some Android NDK versions we need to add this:
// typedef int socklen_t;

#include <android/log.h>
#define LWIP_PLATFORM_ASSERT(x) do { __android_log_print(ANDROID_LOG_ERROR, "husarnet", "Assertion \"%s\" failed at line %d in %s\n", x, __LINE__, __FILE__); abort(); } while(0)
#endif

#include <errno.h>
#include "lwipopts.h"
#include "lwip/debug.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void* ngsocketNetif;
#ifdef __cplusplus
}
#endif

#define LWIP_HOOK_IP6_ROUTE(a, b)       ngsocketNetif

#define TCP_WND 0xFFFF * 4
#define LWIP_WND_SCALE 1
#define TCP_RCV_SCALE 3
#define TCP_SND_SCALE 3

#undef TCP_MSS
#define TCP_MSS 900
#define TCP_SND_BUF 60*TCP_MSS

//#define MEMP_MEM_MALLOC 1
#define SYS_LIGHTWEIGHT_PROT            1
#define NO_SYS                          0
#define MEM_ALIGNMENT                   8U
#define MEM_SIZE                        64000
#define MEMP_NUM_PBUF                   128
#define MEMP_NUM_RAW_PCB                64
#define MEMP_NUM_UDP_PCB                64
#define MEMP_NUM_TCP_PCB                64
#define MEMP_NUM_TCP_PCB_LISTEN         64
#define MEMP_NUM_TCP_SEG                2048
#define MEMP_NUM_REASSDATA              2
#define MEMP_NUM_ARP_QUEUE              8
#define MEMP_NUM_SYS_TIMEOUT            64
#define MEMP_NUM_NETBUF                 16
#define MEMP_NUM_NETCONN                64
#define MEMP_NUM_TCPIP_MSG_API          64
#define MEMP_NUM_TCPIP_MSG_INPKT        64
#define PBUF_POOL_SIZE                  16384 // ~16 MB
#define LWIP_ARP                        1
#define IP_FORWARD                      0
#define IP_OPTIONS_ALLOWED              1
#define IP_REASSEMBLY                   1
#define IP_FRAG                         1
#define IP_REASS_MAXAGE                 3
#define IP_REASS_MAX_PBUFS              4
#define IP_FRAG_USES_STATIC_BUF         0
#define IP_DEFAULT_TTL                  255
#define LWIP_ICMP                       1
#define LWIP_RAW                        1
#define LWIP_DHCP                       0
#define LWIP_AUTOIP                     0
#define LWIP_SNMP                       0
#define LWIP_IGMP                       0
#define LWIP_DNS                        0
#define LWIP_UDP                        1
#define LWIP_TCP                        1
#define LWIP_LISTEN_BACKLOG             0
#define PBUF_LINK_HLEN                  16
#define PBUF_POOL_BUFSIZE               LWIP_MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_HLEN)
#define LWIP_HAVE_LOOPIF                1
#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     1
#define LWIP_STATS                      0
#define PPP_SUPPORT                     0
#define TCP_QUEUE_OOSEQ                 4
#define TCP_OOSEQ_MAX_PBUFS             4
#define LWIP_TCP_SACK_OUT               1

#define LWIP_IPV6 1
#define LWIP_IPV6_SCOPES 0
#define IPV6_FRAG_COPYHEADER 1

#define LWIP_DEBUG 1
#define IP6_DEBUG 0xFFFF

#endif /* LWIP_LWIPOPTS_H */

// breaks under Android and we anyway include errno.h
#ifdef LWIP_ERRNO_INCLUDE
#undef LWIP_ERRNO_INCLUDE
#endif
