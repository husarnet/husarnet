#ifdef __cplusplus
extern "C" {
#endif

#if defined(__linux__)
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#elif defined(ESP_PLATFORM)
#include <lwip/sockets.h>
#else
#error "unknown platform"
#endif

    void husarnet_generate_id(char* identity, int size);
    void husarnet_sockets_init(const char* identity, bool secure, const char* userAgent);
    void husarnet_get_ip(char ip[16]);

    int husarnet_socket(int domain, int type, int protocol);
    int husarnet_connect(int fd, const struct sockaddr* addr, socklen_t addrlen);
    int husarnet_fcntl(int fd, int op, int value);
    int husarnet_bind(int fd, const struct sockaddr* addr, socklen_t addrlen);
    int husarnet_bind_port(int fd, int port);
    int husarnet_select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
                         struct timeval* timeout);
    int husarnet_write(int fd, const void* buf, size_t count);
    int husarnet_read(int fd, void* buf, size_t count);
    int husarnet_accept(int fd, struct sockaddr* addr, socklen_t* addrlen);
    int husarnet_listen(int fd, int backlog);
    int husarnet_close(int fd);
    int husarnet_shutdown(int fd, int how);
    int husarnet_setsockopt(int fd, int level, int optname, void* optval, socklen_t optlen);
    int husarnet_recv(int fd, void* buf, size_t len, int flags);
    int husarnet_recvfrom(int fd, void* buf, size_t len, int flags,
                          struct sockaddr* addr, socklen_t* addrlen);
    int husarnet_send(int fd, const void* buf, size_t len, int flags);
    int husarnet_sendto(int fd, const void* buf, size_t len, int flags,
                        const struct sockaddr* addr, socklen_t addrlen);

    void husarnet_enable_whitelist();
    void husarnet_disable_whitelist();

    void husarnet_whitelist_add(const char* deviceId);
    void husarnet_whitelist_rm(const char* deviceId);
#ifdef __cplusplus
}
#endif
