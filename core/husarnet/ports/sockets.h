// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// Wrapper around OS sockets
#pragma once

#include <functional>
#include <memory>
#include <vector>

#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <winsock2.h>

#include <iphlpapi.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#endif

#include <condition_variable>
#include <mutex>
#include <thread>

#include "husarnet/ipaddress.h"
#include "husarnet/string_view.h"

#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/vector.h>

namespace OsSocket {
  constexpr int UDP_BUFFER_SIZE = 2000;
  constexpr int QUEUE_SIZE_LIMIT = 3000;
  constexpr int TCP_READ_BUFFER = 2000;

  using PacketCallback = std::function<void(InetAddress, string_view)>;

  struct UdpSocket {
    int fd;
    PacketCallback callback;
  };

  struct CustomSocket {
    int fd;
    std::function<void()> callback;
  };

  // Forward declaration
  class TcpConnection;

  using TcpDataCallback = std::function<void(etl::string_view&)>;
  using TcpErrorCallback =
    std::function<void(std::shared_ptr<TcpConnection>)>;
    
  class TcpConnection {
  public:
    enum class Encapsulation {
      NONE,               // Raw data
      FRAMED_TLS_MASKED,  // SSL magic + length + data
    };

    TcpConnection(Encapsulation transportType = Encapsulation::FRAMED_TLS_MASKED): _encapsulation(transportType) {};

    // Create a new TCP connection
    static std::shared_ptr<TcpConnection> connect(
        InetAddress address,
        TcpDataCallback dataCallback,
        TcpErrorCallback errorCallback,
        Encapsulation transportType = TcpConnection::Encapsulation::FRAMED_TLS_MASKED);


    // TODO: remove c style static functions, use Packet shared ptr to pass data

    // Write a data packet.
    // Returns true if the packet was sent.
    static bool write(std::shared_ptr<TcpConnection> conn, std::string& data);
    static bool write(std::shared_ptr<TcpConnection> conn, etl::ivector<char>& data);

    // Close the connection
    static void close(std::shared_ptr<TcpConnection> conn);

    // Get the encapsulation type, 
    Encapsulation getEncapsulationType() const {
      return _encapsulation;
    }

    int getFd() const {
      return fd;
    }

  private:
    int fd = -1;
    etl::string<TCP_READ_BUFFER> readBuffer;
    
    static const inline etl::string<3> TLS_HEADER = "\x17\x03\x03";
    Encapsulation _encapsulation;
    
    // Used for derefered error callback execution
    bool _hasErrored = false;

    TcpDataCallback dataCallback;
    TcpErrorCallback errorCallback;
    
    void _handleRead(std::shared_ptr<TcpConnection> conn);
    void _handleTLSRead(std::shared_ptr<TcpConnection> conn);

    friend void runOnce(int);
  };

  // Connect to a TCP socket not poll-managed by the sockets module
  // Used for performing simple, one-time calls to various APIs
  int connectUnmanagedTcpSocket(InetAddress addr);
  
  bool
  udpListenUnicast(int port, PacketCallback callback, bool setAsDefault = true);
  void udpSend(InetAddress address, string_view data, int fd = -1);
  bool udpListenMulticast(InetAddress address, PacketCallback callback);
  void udpSendMulticast(InetAddress address, const std::string& data);
  int bindUdpSocket(InetAddress addr, bool reuse);

  void bindCustomFd(int fd, std::function<void()> readyCallback);
  
  InetAddress ipFromSockaddr(struct sockaddr_storage st);


  void runOnce(int timeout);

}  // namespace OsSocket
