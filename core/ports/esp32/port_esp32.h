#define LWIP_COMPAT_SOCKETS 0
#define NGSOCKET_USE_LWIP
#include <lwip/sockets.h>
#include <lwip/netdb.h>
extern "C" {
int lwip_accept_r(int s, struct sockaddr *addr, socklen_t *addrlen);
int lwip_bind_r(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_shutdown_r(int s, int how);
int lwip_getpeername_r (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockname_r (int s, struct sockaddr *name, socklen_t *namelen);
int lwip_getsockopt_r (int s, int level, int optname, void *optval, socklen_t *optlen);
int lwip_setsockopt_r (int s, int level, int optname, const void *optval, socklen_t optlen);
int lwip_close_r(int s);
int lwip_connect_r(int s, const struct sockaddr *name, socklen_t namelen);
int lwip_listen_r(int s, int backlog);
int lwip_recv_r(int s, void *mem, size_t len, int flags);
int lwip_read_r(int s, void *mem, size_t len);
int lwip_recvfrom_r(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
int lwip_send_r(int s, const void *dataptr, size_t size, int flags);
int lwip_sendmsg_r(int s, const struct msghdr *message, int flags);
int lwip_sendto_r(int s, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
int lwip_socket(int domain, int type, int protocol);
int lwip_write_r(int s, const void *dataptr, size_t size);
int lwip_writev_r(int s, const struct iovec *iov, int iovcnt);
int lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
int lwip_ioctl_r(int s, long cmd, void *argp);
int lwip_fcntl_r(int s, int cmd, int val);
}

#define lwip_socket_r lwip_socket
#define lwip_select_r lwip_select
#define lwip_freeaddrinfo_r lwip_freeaddrinfo
#define lwip_getaddrinfo_r lwip_getaddrinfo

#define SOCKFUNC(name) ::lwip_ ## name ## _r

#include <string>

void writeBlob(const char* name, const std::string& data);
std::string readBlob(const char* name);
