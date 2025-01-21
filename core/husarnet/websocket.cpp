// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/websocket.h"

#include <etl/string.h>

#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/logging.h"

#include "husarnet_config.h"
#include "sha1.h"
#include "sodium.h"

void WebSocket::connect(InetAddress addr, etl::istring& endpoint)
{
  this->serverAddr = addr;
  this->serverEndpoint.assign(endpoint);

  this->_connectSocket(addr);
}

void WebSocket::connect(InetAddress addr, const char* endpoint)
{
  this->serverAddr = addr;
  this->serverEndpoint.assign(endpoint);

  this->_connectSocket(addr);
}

void WebSocket::_connectSocket(InetAddress addr)
{
  assert(this->conn == nullptr);
  assert(this->serverEndpoint.is_truncated() == false);

  auto dataCallback = [this](etl::string_view& data) {
    this->_handleRead(data);
  };

  auto errorCallback = [this](std::shared_ptr<OsSocket::TcpConnection> conn) {
    LOG_ERROR("WS connection error, closing");  // TODO: errno
    this->close();
  };

  this->conn = OsSocket::TcpConnection::connect(
      addr, dataCallback, errorCallback,
      OsSocket::TcpConnection::Encapsulation::NONE);

  if(this->conn == nullptr || this->conn->getFd() < 0) {
    LOG_ERROR("WS connect failed: %s", strerror(errno));
    return;
  }

  // Try to initiate WebSocket handshake
  if(!this->_sendClientHandshake()) {
    this->close();
    return;
  }

  this->state = WebSocket::State::WS_CONNECTING;
}

void WebSocket::send(const char* data)
{
  WebSocket::Message message(WebSocket::Message::Opcode::TEXT, true);

  size_t len = strnlen(data, message.data.capacity());
  message.data.assign(data, data + len);

  WebSocket::TransportBuffer buf;
  message.encode(buf);
  this->_sendRaw(buf);
}

void WebSocket::send(etl::istring data)
{
  WebSocket::Message message(WebSocket::Message::Opcode::TEXT, true);

  message.data.assign(data.begin(), data.end());

  WebSocket::TransportBuffer buf;
  message.encode(buf);
  this->_sendRaw(buf);
}

void WebSocket::setOnMessageCallback(
    etl::delegate<void(WebSocket::Message&)> callback)
{
  if(callback.is_valid())
    this->onMessage = callback;
}

void WebSocket::close()
{
  if(this->conn == nullptr) {
    return;
  }

  this->conn->close(conn);

  // Explicitly decrement the reference count
  // Otherwise connection ptr will be destroyed after connecting to another
  // socket in WebSocket::_connectSocket(), locking up resources
  this->conn = nullptr;

  this->state = WebSocket::State::SOCK_CLOSED;
}

// --- Incoming message handling ---

void WebSocket::_handleRead(etl::string_view& buf)
{
  switch(this->state) {
    case WebSocket::State::SOCK_CLOSED:
    case WebSocket::State::WS_CLOSED:
      LOG_ERROR("WS unexpected recv on closed/uninitialized socket");
      return;

    case WebSocket::State::WS_CONNECTING:
      this->_handleHandshake(buf);
      break;

    case WebSocket::State::WS_OPEN:
    case WebSocket::State::WS_CLOSING:
      this->_handleData(buf);
      break;
  }
}

void WebSocket::_handleData(etl::string_view& buf)
{
  WebSocket::Message message;

  // One TCP segment can contain multiple WebSocket messages
  while(buf.size() > 0) {
    size_t consumed = WebSocket::Message::parse(buf, message);
    if(consumed == 0) {
      LOG_ERROR("WS invalid message");
      return;
    }

    buf = buf.substr(consumed);
    // buf.erase(buf.begin(), buf.begin() + consumed);

    switch(message.opcode) {
      case WebSocket::Message::Opcode::TEXT:
      case WebSocket::Message::Opcode::BINARY:
        if(this->onMessage.is_valid())
          this->onMessage(message);
        break;

      case WebSocket::Message::Opcode::PING:
        this->_sendPong(message);
        break;

      case WebSocket::Message::Opcode::PONG:
        break;

      case WebSocket::Message::Opcode::CLOSE:
        this->state = WebSocket::State::WS_CLOSING;
        // Respond with a close frame
        this->_sendClose();
        // Close TCP connection
        this->close();
        break;

      default:
        LOG_ERROR("WS unknown opcode: %d", static_cast<int>(message.opcode));
        break;
    }
  }
}

void WebSocket::_sendPong(WebSocket::Message& message)
{
  message.opcode = WebSocket::Message::Opcode::PONG;
  message.masked = true;
  message.flags.fin = true;

  WebSocket::TransportBuffer buf;

  assert(message.encode(buf));  // Should not fail
  this->_sendRaw(buf);
}

void WebSocket::_sendClose()
{
  WebSocket::Message message(WebSocket::Message::Opcode::CLOSE, true);

  WebSocket::TransportBuffer buf;

  assert(message.encode(buf));  // Should not fail
  this->_sendRaw(buf);
}

// --- Handshake ---

void WebSocket::_handleHandshake(etl::string_view& buf)
{
  HTTPMessage message;
  auto res = message.parse(buf);

  // TODO: handle partial messages
  if(res == HTTPMessage::Result::INVALID ||
     res == HTTPMessage::Result::INCOMPLETE ||
     !this->_verifyServerHandshake(message)) {
    LOG_ERROR("Invalid WS server handshake");
    this->close();
    return;
  }

  if(res == HTTPMessage::Result::OK) {
    LOG_INFO("WS connected to %s", this->serverAddr.str().c_str());
    this->state = WebSocket::State::WS_OPEN;
  }
}

bool WebSocket::_sendClientHandshake()
{
  HTTPMessage message;
  message.request.method = "GET";
  message.request.endpoint = this->serverEndpoint;
  assert(message.request.endpoint.is_truncated() == false);

  message.headers.emplace("Host", this->serverAddr.str().c_str());
  message.headers.emplace("Upgrade", "websocket");
  message.headers.emplace("Connection", "Upgrade");
  message.headers.emplace("Sec-WebSocket-Version", "13");
  message.headers.emplace("User-Agent", "husarnet-daemon/" HUSARNET_VERSION);

  // Generate random nonce
  etl::array<uint8_t, 16> rawNonce;
  randombytes_buf(static_cast<void*>(rawNonce.data()), rawNonce.size());

  sodium_bin2base64(
      this->nonce.data(), this->nonce.size(), rawNonce.data(), rawNonce.size(),
      sodium_base64_VARIANT_ORIGINAL);

  message.headers.emplace("Sec-WebSocket-Key", this->nonce.data());

  // Send handshake to server
  WebSocket::TransportBuffer buffer;
  message.encode(buffer);

  return this->_sendRaw(buffer);
}

bool WebSocket::_verifyServerHandshake(HTTPMessage& message)
{
  if(message.messageType != HTTPMessage::Type::RESPONSE) {
    return false;
  }

  if(message.response.statusCode != 101) {
    return false;
  }

  // Check headers
  if(message.headers["Upgrade"] != "websocket") {
    return false;
  }

  if(message.headers["Connection"] != "Upgrade") {
    return false;
  }

  if(!message.headers.contains("Sec-WebSocket-Accept")) {
    return false;
  }

  // Verify server generated key
  // Server appends a static key to the client nonce and hashes the result

  // Static server key appended to the client nonce, per RFC 6455
  const char* serverKey = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  etl::array<char, 20> serverKeyHash;

  {
    // SHA1(client_nonce + server_key)
    etl::string<64> serverKeyNonce(reinterpret_cast<char*>(this->nonce.data()));
    serverKeyNonce += serverKey;
    SHA1(serverKeyHash.data(), serverKeyNonce.data(), serverKeyNonce.size());
  }

  etl::array<char, 29> serverKeyHashBase64;
  sodium_bin2base64(
      serverKeyHashBase64.data(), serverKeyHashBase64.size(),
      reinterpret_cast<unsigned char*>(serverKeyHash.data()),
      serverKeyHash.size(), sodium_base64_VARIANT_ORIGINAL);

  if(strncmp(
         serverKeyHashBase64.data(),
         message.headers["Sec-WebSocket-Accept"].data(),
         serverKeyHashBase64.size()) != 0) {
    return false;
  }

  return true;
}

// --- Socket operations ---

bool WebSocket::_sendRaw(etl::ivector<char>& data)
{
  assert(this->conn != nullptr);

  if(OsSocket::TcpConnection::write(this->conn, data) == false) {
    LOG_ERROR("WS send failed: %s", strerror(errno));
    return false;
  }

  return true;
}
