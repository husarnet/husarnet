// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/sockets.h"

#include "husarnet/ports/port.h"
#include "husarnet/ports/port_interface.h"

#include "husarnet/gil.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

namespace OsSocket {

  const int UDP_BUFFER_SIZE = 2000;
  const int QUEUE_SIZE_LIMIT = 3000;
  const int TCP_READ_BUFFER = 2000;

#ifdef ENABLE_IPV6
#define AF_INETx AF_INET6
  const bool useV6 = true;
#else
#define AF_INETx AF_INET
  const bool useV6 = false;
#endif

  struct sockaddr_in6 makeSockaddr(InetAddress addr, bool v6 = useV6)
  {
    if(v6) {
      struct sockaddr_in6 s {};
      s.sin6_family = AF_INET6;
      s.sin6_port = htons(addr.port);
      memcpy(&s.sin6_addr, addr.ip.data.data(), 16);
      return s;
    } else {
      struct sockaddr_in s {};
      s.sin_family = AF_INET;
      s.sin_port = htons(addr.port);
      if(!addr || addr.ip.isMappedV4()) {
        memcpy(&s.sin_addr, addr.ip.data.data() + 12, 4);
      }
      struct sockaddr_in6 s6 {};
      memcpy(&s6, &s, sizeof(s));
      return s6;
    }
  }

  InetAddress ipFromSockaddr(struct sockaddr_storage st)
  {
    if(st.ss_family == AF_INET) {
      struct sockaddr_in* st4 = (sockaddr_in*)(&st);
      InetAddress r{};
      r.ip.data[10] = 0xFF;  // ipv4-mapped ipv6
      r.ip.data[11] = 0xFF;
      memcpy((char*)r.ip.data.data() + 12, &st4->sin_addr, 4);
      r.port = htons(st4->sin_port);
      return r;
    } else if(st.ss_family == AF_INET6) {
      struct sockaddr_in6* st6 = (sockaddr_in6*)(&st);
      InetAddress r{};
      memcpy(r.ip.data.data(), &st6->sin6_addr, 16);

      if(r.ip.data[0] == 0 && r.ip.data[1] == 0x64 && r.ip.data[2] == 0xff &&
         r.ip.data[3] == 0x9b) {
        // NAT64 mapped IPv4 address, do our best at mapping it
        memset(&r.ip.data[0], 0, 10);
        r.ip.data[10] = 0xFF;
        r.ip.data[11] = 0xFF;
      }

      r.port = htons(st6->sin6_port);
      return r;
    } else {
      return InetAddress();
    }
  }

  struct UdpSocket {
    int fd;
    PacketCallack callback;
  };

  struct CustomSocket {
    int fd;
    std::function<void()> callback;
  };

  std::vector<UdpSocket> udpSockets;
  std::vector<CustomSocket> customSockets;
  int unicastUdpFd = -1;
  int multicastUdpFd4 = -1;
  int multicastUdpFd6 = -1;

  void set_nonblocking(int fd)
  {
#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = SOCKFUNC(fcntl)(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    SOCKFUNC(fcntl)(fd, F_SETFL, flags);
#endif
  }

  void set_blocking(int fd)
  {
#ifdef _WIN32
    unsigned long mode = 0;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = SOCKFUNC(fcntl)(fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    SOCKFUNC(fcntl)(fd, F_SETFL, flags);
#endif
  }

  int bindUdpSocket(InetAddress addr, bool reuse, bool v6)
  {
    int fd = SOCKFUNC(socket)(useV6 ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
      LOG_CRITICAL("creating socket failed with %d", (int)errno);
      return -1;
    }
    if(reuse) {
      int one = 1;
      SOCKFUNC(setsockopt)
      (fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(one));
    }

    set_nonblocking(fd);

    auto sa = makeSockaddr(addr, useV6);
    socklen_t socklen = sizeof(sa);
    if(SOCKFUNC(bind)(fd, (sockaddr*)&sa, socklen) < 0) {
      SOCKFUNC_close(fd);
      return -1;
    }
    return fd;
  }

  int bindUdpSocket(InetAddress addr, bool reuse)
  {
    return bindUdpSocket(addr, reuse, useV6);
  }

  void bindCustomFd(int fd, std::function<void()> readyCallback)
  {
    customSockets.push_back(CustomSocket{fd, readyCallback});
  }

  bool udpListenUnicast(int port, PacketCallack callback, bool setAsDefault)
  {
    int fd = bindUdpSocket(InetAddress{IpAddress(), (uint16_t)port}, false);
    if(fd == -1)
      return false;

    if(setAsDefault)
      unicastUdpFd = fd;

    udpSockets.push_back(UdpSocket{fd, callback});

    return true;
  }

  void udpSend(InetAddress address, string_view data, int fd)
  {
    if(fd == -1) {
      if(unicastUdpFd == -1)
        return;
      fd = unicastUdpFd;
    }
    auto sa = makeSockaddr(address);
    socklen_t socklen = sizeof(sa);
    SOCKFUNC(sendto)(fd, data.data(), data.size(), 0, (sockaddr*)&sa, socklen);
  }

  bool udpListenMulticast(InetAddress address, PacketCallack callback)
  {
    if(address.ip.isMappedV4()) {
      int fd = bindUdpSocket(
          InetAddress{IpAddress(), (uint16_t)address.port}, true,
          /*ipv6=*/false);
      if(fd == -1)
        return false;
      multicastUdpFd4 = fd;

      struct ip_mreq mreq {};
      memcpy(&mreq.imr_multiaddr, address.ip.data.data() + 12, 4);

      if(SOCKFUNC(setsockopt)(
             fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq,
             sizeof(mreq)) == 0) {
        udpSockets.push_back(UdpSocket{fd, callback});
        return true;
      } else {
        return false;
      }
    } else {
      int fd =
          bindUdpSocket(InetAddress{IpAddress(), (uint16_t)address.port}, true);
      if(fd == -1)
        return false;
      multicastUdpFd6 = fd;

#ifndef ESP_PLATFORM
      struct ipv6_mreq mreq {};
      memcpy(&mreq.ipv6mr_multiaddr, address.ip.data.data(), 16);
      mreq.ipv6mr_interface = 0;
      if(SOCKFUNC(setsockopt)(
             fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char*)&mreq,
             sizeof(mreq)) == 0) {
        udpSockets.push_back(UdpSocket{fd, callback});
        return true;
      } else {
        return false;
      }
#else
      return false;
#endif
    }
  }

  void udpSendMulticast(InetAddress address, const std::string& data)
  {
    auto sa = makeSockaddr(address);
    socklen_t socklen = sizeof(sa);

    if(address.ip.isMappedV4()) {
      if(multicastUdpFd4 == -1)
        return;
      SOCKFUNC(sendto)
      (multicastUdpFd4, data.data(), data.size(), 0, (sockaddr*)&sa, socklen);
    } else {
      if(multicastUdpFd6 == -1)
        return;
      SOCKFUNC(sendto)
      (multicastUdpFd6, data.data(), data.size(), 0, (sockaddr*)&sa, socklen);
    }
  }

  // TCP

  int connectTcpSocket(InetAddress addr)
  {
    int fd = SOCKFUNC(socket)(AF_INETx, SOCK_STREAM, 0);
    if(fd < 0) {
      LOG_CRITICAL("creating socket failed with %d", (int)errno);
      return -1;
    }

    auto sa = makeSockaddr(addr, useV6);
    socklen_t socklen = sizeof(sa);

    // Set socket to nonblocking to be able to check if connect has stalled
    set_nonblocking(fd);

    int res = SOCKFUNC(connect)(fd, (sockaddr*)&sa, socklen);

    if(res < 0 && errno != EINPROGRESS) {
      LOG_ERROR("connection with the server (%s) failed", addr.str().c_str());
      SOCKFUNC_close(fd);
      return -1;
    }

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    struct timeval tv {
      .tv_sec = 5,
      .tv_usec = 0,
    };

    res = select(fd + 1, NULL, &fdset, NULL, &tv);

    if (res <= 0) {
      LOG_ERROR("connection with the server (%s) failed (timeout)", addr.str().c_str());
      SOCKFUNC_close(fd);
      return -1;
    }

    else {
      #ifdef _WIN32
      char so_error;
      #else
      int so_error;
      #endif
      
      socklen_t len = sizeof(so_error);

      SOCKFUNC(getsockopt)(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

      if (so_error != 0) {
        LOG_ERROR("connection with the server (%s) failed (error)", addr.str().c_str());
        SOCKFUNC_close(fd);
        return -1;
      }
    }

    // Set socket back to blocking
    set_blocking(fd);

    return fd;
  }

  struct FramedTcpConnection {
    int fd = -1;
    std::string queue;
    std::string readBuffer;
    int readBufferPtr = 0;
    int queuePtr = 0;
    bool connected = false;

    TcpDataCallback dataCallback;
    TcpErrorCallback errorCallback;

    FramedTcpConnection()
    {
      readBuffer.resize(TCP_READ_BUFFER);
    }
  };

  std::vector<std::shared_ptr<FramedTcpConnection> > tcpConnections;
  std::vector<std::shared_ptr<FramedTcpConnection> > tcpConnectionsErrored;

  bool write(
      std::shared_ptr<FramedTcpConnection> conn,
      const std::string& packet,
      bool doQueue)
  {
    if(conn->fd == -1)
      return false;
    assert(packet.size() > 0);

    // Masquerade our stream as SSL.
    std::string header = "\x17\x03\x03" + pack((uint16_t)packet.size());
    std::string data = header + packet;

    if(conn->queue.size() != 0 || !conn->connected) {
      if(conn->queue.size() + data.size() > QUEUE_SIZE_LIMIT) {
        return false;
      }

      if(doQueue) {
        conn->queue += data;
        return true;
      } else {
        return false;
      }
    }

    long wrote = SOCKFUNC(send)(conn->fd, data.data(), data.size(), 0);
    if(wrote < 0)
      wrote = 0;

    if(wrote == (long)data.size())
      return true;

    if(wrote != 0 || doQueue) {
      conn->queue = data.substr(wrote);
      return true;
    }

    return false;
  }

  void writeQueue(std::shared_ptr<FramedTcpConnection> conn)
  {
    if(conn->fd == -1)
      return;

    long wrote = SOCKFUNC(send)(
        conn->fd, conn->queue.data() + conn->queuePtr,
        (long)conn->queue.size() - conn->queuePtr, 0);
    if(wrote < 0)
      wrote = 0;
    conn->queuePtr += (int)wrote;

    if(conn->queuePtr == (int)conn->queue.size()) {
      conn->queue = "";
      conn->queuePtr = 0;
    }
  }

  void handleRead(std::shared_ptr<FramedTcpConnection> conn)
  {
    while(true) {
      long readSize = (long)conn->readBuffer.size() - conn->readBufferPtr;
      assert(readSize >= 0);
      long rd = 0;
      if(readSize != 0) {
        rd = SOCKFUNC(recv)(
            conn->fd, (char*)conn->readBuffer.data() + conn->readBufferPtr,
            readSize, 0);

        if(rd == 0 || (rd < 0 && errno != EAGAIN)) {
          close(conn);  // closed
          return;
        }
        if(rd < 0 && errno == EAGAIN)
          return;
      }

      conn->readBufferPtr += (int)rd;

      while(true) {
        // LOG("read buffer is (%d)", (int)conn->readBufferPtr);
        if(conn->readBufferPtr <= 5)
          return;

        if(conn->readBuffer.substr(0, 3) != "\x17\x03\x03") {
          LOG_ERROR("TCP socket format error (1)");
          close(conn);  // oops
          return;
        }

        uint16_t expectedSize = unpack<uint16_t>(conn->readBuffer.substr(3, 2));
        if(expectedSize + 5 >= (int)conn->readBuffer.size()) {
          LOG_INFO(
              "TCP message too large (%d, max is %d)", expectedSize,
              (int)conn->readBuffer.size());
          close(conn);  // oops
          return;
        }

        if(expectedSize + 5 <= conn->readBufferPtr) {
          std::string packet = conn->readBuffer.substr(5, expectedSize);
          conn->dataCallback(packet);
          int newPtr = 5 + expectedSize;
          memmove(
              &conn->readBuffer[0], &conn->readBuffer[newPtr],
              conn->readBufferPtr - newPtr);
          conn->readBufferPtr -= newPtr;
        } else {
          break;
        }
      }
#ifdef ESP_PLATFORM
      break;  // nonblocking mode doesn't work on lwIP
#endif
    }
  }

  void close(std::shared_ptr<FramedTcpConnection> conn)
  {
    if(conn->fd == -1)
      return;

    SOCKFUNC_close(conn->fd);
    conn->fd = -1;
    tcpConnectionsErrored.push_back(conn);
  }

  std::shared_ptr<FramedTcpConnection> tcpConnect(
      InetAddress address,
      TcpDataCallback dataCallback,
      TcpErrorCallback errorCallback)
  {
    auto conn = std::make_shared<FramedTcpConnection>();
    conn->dataCallback = dataCallback;
    conn->errorCallback = errorCallback;
    conn->fd = SOCKFUNC(socket)(AF_INETx, SOCK_STREAM, 0);

    int val = 1;
    SOCKFUNC(setsockopt)
    (conn->fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&val, sizeof(int));

    set_nonblocking(conn->fd);

    auto sa = makeSockaddr(address);
    SOCKFUNC(connect)(conn->fd, (sockaddr*)(&sa), sizeof(sa));  // ignore result

    tcpConnections.push_back(conn);
    return conn;
  }

  std::string udpBuffer;

  void runOnce(int timeout)
  {
    fd_set readset;
    fd_set writeset;
    fd_set errorset;
    FD_ZERO(&writeset);
    FD_ZERO(&readset);
    FD_ZERO(&errorset);
    int maxfd = 0;

    for(auto conn : tcpConnections) {
      if(conn->fd <= 0)
        continue;
      if(conn->queue.size() > 0 || !conn->connected) {
        FD_SET(conn->fd, &writeset);
      }
      FD_SET(conn->fd, &readset);
      FD_SET(conn->fd, &errorset);
      maxfd = std::max(conn->fd, maxfd);
    }

    for(auto conn : udpSockets) {
      FD_SET(conn.fd, &readset);
      maxfd = std::max(conn.fd, maxfd);
    }

    for(auto conn : customSockets) {
      FD_SET(conn.fd, &readset);
      FD_SET(conn.fd, &errorset);
      maxfd = std::max(conn.fd, maxfd);
    }

    GIL::unlocked<void>([&]() {
      struct timeval timeoutval;
      timeoutval.tv_sec = timeout / 1000;
      timeoutval.tv_usec = (timeout % 1000) * 1000;

      errno = 0;

      int res = SOCKFUNC(select)(
          maxfd + 1, &readset, &writeset, &errorset,
          &timeoutval);  // ignore result
      (void)res;
    });

    for(auto& conn : udpSockets) {
      if(conn.fd == -1)
        continue;

      if(FD_ISSET(conn.fd, &readset)) {
        struct sockaddr_storage s;
        socklen_t len = sizeof(s);
        udpBuffer.resize(UDP_BUFFER_SIZE);

        while(true) {
          long r = SOCKFUNC(recvfrom)(
              conn.fd, &udpBuffer[0], udpBuffer.size(), 0, (sockaddr*)&s, &len);
          if(r <= 0)
            break;
          conn.callback(ipFromSockaddr(s), string_view(udpBuffer).substr(0, r));
        }
      }
    }

    for(auto& conn : customSockets) {
      if(conn.fd == -1)
        continue;

      if(FD_ISSET(conn.fd, &readset) || FD_ISSET(conn.fd, &errorset)) {
        conn.callback();
      }

      if(FD_ISSET(conn.fd, &errorset)) {
      }
    }

    for(auto conn : tcpConnections) {
      if(conn->fd == -1)
        continue;

      if(FD_ISSET(conn->fd, &writeset)) {
        conn->connected = true;
        writeQueue(conn);
      }

      if(FD_ISSET(conn->fd, &readset) || FD_ISSET(conn->fd, &errorset)) {
        handleRead(conn);
      }
    }

    auto tcpConnectionsErroredCopy = tcpConnectionsErrored;
    for(auto conn : tcpConnectionsErroredCopy) {
      auto it = std::find(tcpConnections.begin(), tcpConnections.end(), conn);
      if(it != tcpConnections.end()) {
        conn->errorCallback(conn);
        tcpConnections.erase(it);
      }
    }
    tcpConnectionsErrored.clear();
  }

}  // namespace OsSocket
