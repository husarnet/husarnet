set(D ${CMAKE_CURRENT_LIST_DIR})
set(LWIPDIR ${D}/lwip/src)
set(LWIPCONTRIBDIR ${D}/lwip-contrib)
#include_directories(../lwip-contrib/ports/unix/lib)

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  include_directories(${D}/lwip-contrib/ports/win32/include)
else()
  include_directories(${D}/lwip-contrib/ports/unix/port/include)
endif()

include_directories(${D}/lwip-config/)
include_directories(${LWIPDIR}/include)

set(LWIP_SOURCE
  ${LWIPDIR}/core/init.c
  ${LWIPDIR}/core/def.c
  ${LWIPDIR}/core/dns.c
  ${LWIPDIR}/core/inet_chksum.c
  ${LWIPDIR}/core/ip.c
  ${LWIPDIR}/core/mem.c
  ${LWIPDIR}/core/memp.c
  ${LWIPDIR}/core/netif.c
  ${LWIPDIR}/core/pbuf.c
  ${LWIPDIR}/core/raw.c
  ${LWIPDIR}/core/stats.c
  ${LWIPDIR}/core/sys.c
  ${LWIPDIR}/core/altcp.c
  ${LWIPDIR}/core/altcp_tcp.c
  ${LWIPDIR}/core/tcp.c
  ${LWIPDIR}/core/tcp_in.c
  ${LWIPDIR}/core/tcp_out.c
  ${LWIPDIR}/core/timeouts.c
  ${LWIPDIR}/core/udp.c
  ${LWIPDIR}/core/ipv4/autoip.c
  ${LWIPDIR}/core/ipv4/dhcp.c
  ${LWIPDIR}/core/ipv4/etharp.c
  ${LWIPDIR}/core/ipv4/icmp.c
  ${LWIPDIR}/core/ipv4/igmp.c
  ${LWIPDIR}/core/ipv4/ip4_frag.c
  ${LWIPDIR}/core/ipv4/ip4.c
  ${LWIPDIR}/core/ipv4/ip4_addr.c
  ${LWIPDIR}/core/ipv6/dhcp6.c
  ${LWIPDIR}/core/ipv6/ethip6.c
  ${LWIPDIR}/core/ipv6/icmp6.c
  ${LWIPDIR}/core/ipv6/inet6.c
  ${LWIPDIR}/core/ipv6/ip6.c
  ${LWIPDIR}/core/ipv6/ip6_addr.c
  ${LWIPDIR}/core/ipv6/ip6_frag.c
  ${LWIPDIR}/core/ipv6/mld6.c
  ${LWIPDIR}/core/ipv6/nd6.c
  ${LWIPDIR}/api/api_lib.c
  ${LWIPDIR}/api/api_msg.c
  ${LWIPDIR}/api/err.c
  ${LWIPDIR}/api/if_api.c
  ${LWIPDIR}/api/netbuf.c
  ${LWIPDIR}/api/netdb.c
  ${LWIPDIR}/api/netifapi.c
  ${LWIPDIR}/api/sockets.c
  ${LWIPDIR}/api/tcpip.c
  ${LWIPDIR}/netif/ethernet.c
  ${LWIPDIR}/netif/bridgeif.c
  ${LWIPDIR}/netif/slipif.c)

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)

else()
  set(LWIP_SOURCE
    ${LWIP_SOURCE}
    ${LWIPCONTRIBDIR}/ports/unix/port/sys_arch.c)
endif()
