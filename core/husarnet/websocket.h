// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <memory>

#include <etl/delegate.h>
#include <etl/string.h>
#include <etl/vector.h>

#include "husarnet/ports/sockets.h"

#include "husarnet/http.h"
#include "husarnet/ipaddress.h"

class WebSocket {
 public:
  enum class State
  {
    SOCK_CLOSED = 0,
    WS_CLOSED,
    WS_CONNECTING,
    WS_OPEN,
    WS_CLOSING,
  };

  class Message {
   public:
    class Flags {
     public:
      bool fin : 1 = 0;
      bool rsv1 : 1 = 0;
      bool rsv2 : 1 = 0;
      bool rsv3 : 1 = 0;
    };

    enum class Opcode : uint8_t
    {
      CONTINUATION = 0x0,
      TEXT = 0x1,
      BINARY = 0x2,
      CLOSE = 0x8,
      PING = 0x9,
      PONG = 0xA,
    };

    Message() : flags({.fin = true}), opcode(Message::Opcode::TEXT), masked(true){};
    Message(Message::Opcode opcode, bool masked) : flags({.fin = true}), opcode(opcode), masked(masked){};

    // Parse message from buffer to provided message object
    // @returns number of bytes consumed, 0 if parser failure occurred
    static size_t parse(etl::string_view& buffer, Message& message);

    // Encode message to buffer
    bool encode(etl::ivector<char>& buffer) const;

    size_t payloadSize() const
    {
      return data.size();
    }

    Flags flags;
    Opcode opcode;
    bool masked;
    etl::vector<char, 128> data;
  };

  using TransportBuffer = etl::vector<char, 1024>;
  using OnMessageDelegate = etl::delegate<void(WebSocket::Message&)>;

  WebSocket(){};

  void connect(InetAddress addr, etl::istring& endpoint);
  void connect(InetAddress addr, const char* endpoint);
  void send(etl::istring msg);
  void send(const char* msg);
  void setOnMessageCallback(OnMessageDelegate callback);
  void close();
  WebSocket::State getState()
  {
    return this->state;
  }

 private:
  void _connectSocket(InetAddress addr);
  bool _sendClientHandshake();
  bool _verifyServerHandshake(HTTPMessage& message);

  void _handleRead(etl::string_view& buf);
  void _handleHandshake(etl::string_view& buf);
  void _handleData(etl::string_view& buf);
  void _sendPong(WebSocket::Message& message);
  void _sendClose();

  bool _sendRaw(etl::ivector<char>& data);

  std::shared_ptr<OsSocket::TcpConnection> conn;

  InetAddress serverAddr;
  etl::string<128> serverEndpoint;
  std::array<char, 25> nonce;

  WebSocket::State state = WebSocket::State::SOCK_CLOSED;

  etl::delegate<void(WebSocket::Message&)> onMessage;
};
