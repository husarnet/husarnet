// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <etl/delegate.h>
#include <etl/string.h>
#include <etl/vector.h>

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

    Message()
        : flags({.fin = true}), opcode(Message::Opcode::TEXT), masked(true){};
    Message(Message::Opcode opcode, bool masked)
        : flags({.fin = true}), opcode(opcode), masked(masked){};

    // Parse message from buffer to provided message object
    // @returns number of bytes consumed, 0 if parser failure occured
    static size_t parse(etl::ivector<char>& buffer, Message& message);

    // Encode message to buffer
    void encode(etl::ivector<char>& buffer);

    size_t payloadSize() const
    {
      return data.size();
    }

    Flags flags;
    Opcode opcode;
    bool masked;
    etl::vector<char, 128> data;
  };

  // Buffer used for socket operations and HTTP handshake
  using TransportBuffer = etl::vector<char, 256>;

  WebSocket(){};

  void connect(InetAddress addr, etl::istring endpoint);
  void connect(InetAddress addr, const char* endpoint);
  void send(etl::istring msg);
  void send(const char* msg);
  void setOnMessageCallback(etl::delegate<void(WebSocket::Message&)> callback);
  void periodic(int timeout);
  void close();
  WebSocket::State getState()
  {
    return this->state;
  }

 private:
  bool _sendClientHandshake();
  bool _verifyServerHandshake(HTTPMessage& message);

  void _handleRead(etl::ivector<char>& buf);
  void _handleHandshake(etl::ivector<char>& buf);
  void _handleData(etl::ivector<char>& buf);
  void _sendPong(WebSocket::Message& message);
  void _sendClose();

  void _sendRaw(etl::ivector<char>& buf);
  void _readRaw();
  void _select(int timeout);

  int fd = -1;

  InetAddress serverAddr;
  etl::string<32> serverEndpoint;
  std::array<char, 25> nonce;

  WebSocket::State state = WebSocket::State::SOCK_CLOSED;

  etl::delegate<void(WebSocket::Message&)> onMessage;
};
