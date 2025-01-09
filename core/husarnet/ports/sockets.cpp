// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/sockets.h"

#include "husarnet/ports/port.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

namespace OsSocket {

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

  std::vector<UdpSocket> udpSockets;
  std::vector<CustomSocket> customSockets;
  std::vector<std::shared_ptr<TcpConnection>> tcpConnections;
  int unicastUdpFd = -1;
  int multicastUdpFd4 = -1;
  int multicastUdpFd6 = -1;

  void set_nonblocking(int fd)
  {
#ifdef PORT_WINDOWS
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
#ifdef PORT_WINDOWS
    unsigned long mode = 0;
    ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = SOCKFUNC(fcntl)(fd, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    SOCKFUNC(fcntl)(fd, F_SETFL, flags);
#endif
  }

  // -----
  // UDP
  // -----

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

  bool udpListenUnicast(int port, PacketCallback callback, bool setAsDefault)
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

  bool udpListenMulticast(InetAddress address, PacketCallback callback)
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

  // -----
  // Custom
  // -----

  void bindCustomFd(int fd, std::function<void()> readyCallback)
  {
    customSockets.push_back(CustomSocket{fd, readyCallback});
  }

  // -----
  // TCP
  // -----

  int connectUnmanagedTcpSocket(InetAddress addr)
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
      .tv_sec = 5, .tv_usec = 0,
    };

    // Wait for the socket to become writable
    res = select(fd + 1, NULL, &fdset, NULL, &tv);

    if(res <= 0) {
      LOG_ERROR(
          "connection with the server (%s) failed (timeout)",
          addr.str().c_str());
      SOCKFUNC_close(fd);
      return -1;
    }

    else {
#ifdef PORT_WINDOWS
      char so_error;
#else
      int so_error;
#endif

      socklen_t len = sizeof(so_error);

      SOCKFUNC(getsockopt)(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

      if(so_error != 0) {
        LOG_ERROR(
            "connection with the server (%s) failed (error)",
            addr.str().c_str());
        SOCKFUNC_close(fd);
        return -1;
      }
    }

    // Set socket back to blocking
    set_blocking(fd);

    return fd;
  }

  bool TcpConnection::write(
      std::shared_ptr<TcpConnection> conn,
      std::string& packet)
  {
    if(conn->fd == -1)
      return false;
    assert(packet.size() > 0);

    if(conn->getEncapsulationType() == Encapsulation::FRAMED_TLS_MASKED) {
      // Masquerade our stream as SSL.
      std::string header = "\x17\x03\x03" + pack((uint16_t)packet.size());
      packet.insert(0, header);
    }

    ssize_t bytes_written = 0;

    // Send data supporting partial writes
    do {
      ssize_t remaining = packet.size() - bytes_written;
      ssize_t n =
          SOCKFUNC(send)(conn->fd, packet.data() + bytes_written, remaining, 0);

      if(n < 0) {
        LOG_ERROR("WS send failed: %s", strerror(errno));
        return false;
      }

      bytes_written += n;
    } while(bytes_written != packet.size());

    return true;
  }

  bool TcpConnection::write(
      std::shared_ptr<TcpConnection> conn,
      etl::ivector<char>& packet)
  {
    if(conn->fd == -1)
      return false;
    assert(packet.size() > 0);

    if(conn->getEncapsulationType() == Encapsulation::FRAMED_TLS_MASKED) {
      // Masquerade our stream as SSL.
      std::string header = "\x17\x03\x03" + pack((uint16_t)packet.size());

      if(packet.size() + header.size() > packet.capacity()) {
        LOG_ERROR(
            "TCP message too large (%d, max is %d)", packet.size(),
            packet.capacity());
        return false;
      }

      packet.insert(packet.begin(), header.begin(), header.end());
    }

    ssize_t bytes_written = 0;

    // Send data supporting partial writes
    do {
      ssize_t remaining = packet.size() - bytes_written;
      ssize_t n =
          SOCKFUNC(send)(conn->fd, packet.data() + bytes_written, remaining, 0);

      if(n < 0) {
        LOG_ERROR("TCP send failed: %s", strerror(errno));
        return false;
      }

      bytes_written += n;
    } while(bytes_written != packet.size());

    return true;
  }

  void TcpConnection::_handleRead(std::shared_ptr<TcpConnection> conn)
  {
    while(true) {
      ssize_t read = 0;

      size_t available = conn->readBuffer.available();
      if(available > 0) {
        // Read data into buffer memory
        // Buffer must be shrunk from the maximum size to the actual read size
        // to prevent reinitialization of the buffer underlying data type
        size_t size = conn->readBuffer.size();
        conn->readBuffer.uninitialized_resize(conn->readBuffer.capacity());
        read = SOCKFUNC(recv)(
            conn->fd, conn->readBuffer.begin() + size, available, 0);

        size_t newSize = (read > 0) ? size + read : size;
        conn->readBuffer.resize(newSize);

        if(read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
          return;
        }

        if(read <= 0) {
          LOG_ERROR("TCP recv failed: %s", strerror(errno));
          TcpConnection::close(conn);
          return;
        }
      }

      // Pass data directly to the callback if no encapsulation is used
      if(this->getEncapsulationType() == Encapsulation::NONE) {
        auto view = etl::string_view(conn->readBuffer);
        conn->dataCallback(view);
        conn->readBuffer.clear();
      } else {
        while(true) {
          // TODO: refactor: packet pool

          // Check if we have a full packet
          if(conn->readBuffer.size() <= 5)
            return;

          auto iter = conn->readBuffer.begin();

          // Is packet masquerading as SSL?
          if(etl::string_view(iter, 3) != conn->TLS_HEADER) {
            LOG_ERROR("TCP socket format error (1)");
            TcpConnection::close(conn);
            return;
          }

          iter += conn->TLS_HEADER.size();

          // Extract packet size
          uint16_t expectedSize = unpack<uint16_t>(etl::string_view(iter, 2));
          iter += 2;

          size_t packetLen =
              etl::distance(conn->readBuffer.begin(), iter) + expectedSize;
          if(packetLen >= conn->readBuffer.capacity()) {
            LOG_INFO(
                "TCP message too large (%d, max is %d)", expectedSize,
                conn->readBuffer.size());
            TcpConnection::close(conn);
            return;
          }

          if(conn->readBuffer.size() >= packetLen) {
            etl::string_view packet(conn->readBuffer.begin() + 5, expectedSize);
            conn->dataCallback(packet);

            // Remove the packet from the buffer
            conn->readBuffer.erase(0, packetLen);
          } else {
            // We don't have the full packet yet
            break;
          }
        }
      }

#ifdef ESP_PLATFORM
      break;  // nonblocking mode doesn't work on lwIP
#endif
    }
  }

  void TcpConnection::close(std::shared_ptr<TcpConnection> conn)
  {
    if(conn->fd != -1)
      SOCKFUNC_close(conn->fd);

    conn->fd = -1;
    conn->_hasErrored = true;
  }

  // TODO: change callbacks from func pointers to etl::delegate
  std::shared_ptr<TcpConnection> TcpConnection::connect(
      const InetAddress& address,
      TcpDataCallback dataCallback,
      TcpErrorCallback errorCallback,
      TcpConnection::Encapsulation transportType)
  {
    auto conn = std::make_shared<TcpConnection>(transportType);
    conn->dataCallback = dataCallback;
    conn->errorCallback = errorCallback;
    conn->fd = SOCKFUNC(socket)(AF_INETx, SOCK_STREAM, 0);

    if(conn->fd < 0) {
      LOG_CRITICAL("creating socket failed with %d", (int)errno);
      return nullptr;
    }

    int val = 1;
    SOCKFUNC(setsockopt)
    (conn->fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&val, sizeof(int));

    set_nonblocking(conn->fd);

    auto sa = makeSockaddr(address);
    socklen_t socklen = sizeof(sa);

    int res = SOCKFUNC(connect)(conn->fd, (sockaddr*)(&sa), socklen);

    if(res < 0 && errno != EINPROGRESS) {
      LOG_ERROR(
          "connection with the server (%s) failed", address.str().c_str());
      SOCKFUNC_close(conn->fd);
      return nullptr;
    }

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(conn->fd, &fdset);

    struct timeval tv {
      .tv_sec = 5, .tv_usec = 0,
    };

    // Wait for the socket to become writable i.e. connect has succeeded
    res = select(conn->fd + 1, NULL, &fdset, NULL, &tv);

    if(res <= 0) {
      LOG_ERROR(
          "connection with the server (%s) failed (timeout)",
          address.str().c_str());
      SOCKFUNC_close(conn->fd);
      return nullptr;
    }

    else {
#ifdef PORT_WINDOWS
      char so_error;
#else
      int so_error;
#endif

      socklen_t len = sizeof(so_error);

      SOCKFUNC(getsockopt)(conn->fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

      if(so_error != 0) {
        LOG_ERROR(
            "connection with the server (%s) failed (error)",
            address.str().c_str());
        SOCKFUNC_close(conn->fd);
        return nullptr;
      }
    }

    tcpConnections.push_back(conn);
    return conn;
  }

  std::string udpBuffer;

  void runOnce(int timeout)
  {
    fd_set readset;
    FD_ZERO(&readset);
    int maxfd = 0;

    for(auto conn : tcpConnections) {
      if(conn->fd <= 0)
        continue;
      FD_SET(conn->fd, &readset);
      maxfd = std::max(conn->fd, maxfd);
    }

    for(auto conn : udpSockets) {
      FD_SET(conn.fd, &readset);
      maxfd = std::max(conn.fd, maxfd);
    }

    for(auto conn : customSockets) {
      FD_SET(conn.fd, &readset);
      maxfd = std::max(conn.fd, maxfd);
    }

    struct timeval timeoutval;
    timeoutval.tv_sec = timeout / 1000;
    timeoutval.tv_usec = (timeout % 1000) * 1000;

    errno = 0;

    int res = SOCKFUNC(select)(maxfd + 1, &readset, NULL, NULL, &timeoutval);

    if(res < 0 && errno != EINTR) {
      LOG_ERROR("select failed: %s", strerror(errno));
      return;
    }

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

      if(FD_ISSET(conn.fd, &readset)) {
        conn.callback();
      }
    }

    for(auto conn : tcpConnections) {
      if(conn->fd == -1)
        continue;

      if(FD_ISSET(conn->fd, &readset)) {
        conn->_handleRead(conn);
      }
    }

    // Execute error callbacks on errored connections
    for(auto conn : tcpConnections) {
      if(conn->_hasErrored) {
        conn->errorCallback(conn);
      }
    }

    // Remove errored connection entries as they have been closed
    tcpConnections.erase(
        std::remove_if(
            tcpConnections.begin(), tcpConnections.end(),
            [](std::shared_ptr<TcpConnection> conn) {
              return conn->_hasErrored;
            }),
        tcpConnections.end());
  }

}  // namespace OsSocket
