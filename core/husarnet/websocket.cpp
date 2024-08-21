// Copyright (c) 2024 Husarnet sp. z o.o.
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

void WebSocket::connect(InetAddress addr, etl::istring endpoint)
{
  this->serverAddr = addr;
  this->serverEndpoint = endpoint;

  assert(this->serverEndpoint.is_truncated() == false);

  this->fd = OsSocket::connectTcpSocket(addr);
  if(this->fd < 0) {
    LOG_ERROR("WS connect failed: %s", strerror(errno));
    return;
  }

  this->state = WebSocket::State::WS_CONNECTING;
  this->_sendClientHandshake();
}

void WebSocket::connect(InetAddress addr, const char* endpoint)
{
  this->serverAddr = addr;
  this->serverEndpoint.assign(endpoint);

  assert(this->serverEndpoint.is_truncated() == false);

  this->fd = OsSocket::connectTcpSocket(addr);
  if(this->fd < 0) {
    LOG_ERROR("WS connect failed: %s", strerror(errno));
    return;
  }

  this->state = WebSocket::State::WS_CONNECTING;
  this->_sendClientHandshake();
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
  if(this->fd < 0) {
    return;
  }

  SOCKFUNC_close(this->fd);
  this->fd = -1;

  this->state = WebSocket::State::SOCK_CLOSED;
}

// --- Incoming message handling ---

void WebSocket::_handleRead(etl::ivector<char>& buf)
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

void WebSocket::_handleData(etl::ivector<char>& buf)
{
  WebSocket::Message message;

  // One TCP segment can contain multiple WebSocket messages
  while(buf.size() > 0) {
    size_t consumed = WebSocket::Message::parse(buf, message);
    if(consumed == 0) {
      LOG_ERROR("WS invalid message");
      return;
    }

    buf.erase(buf.begin(), buf.begin() + consumed);

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

  message.encode(buf);
  this->_sendRaw(buf);
}

void WebSocket::_sendClose()
{
  WebSocket::Message message(WebSocket::Message::Opcode::CLOSE, true);

  WebSocket::TransportBuffer buf;

  message.encode(buf);
  this->_sendRaw(buf);
}

// --- Handshake ---

void WebSocket::_handleHandshake(etl::ivector<char>& buf)
{
  HTTPMessage message;

  if(!HTTPMessage::parse(buf, message) ||
     !this->_verifyServerHandshake(message)) {
    LOG_ERROR("Invalid WS server handshake");
    this->close();
    return;
  }

  LOG_INFO("WS connected to %s", this->serverAddr.str().c_str());

  this->state = WebSocket::State::WS_OPEN;
}

bool WebSocket::_sendClientHandshake()
{
  HTTPMessage message;
  message.request.method = "GET";
  message.request.endpoint = this->serverEndpoint;
  assert(message.request.endpoint.is_truncated() == false);

  message.headers.insert(
      etl::make_pair("Host", this->serverAddr.str().c_str()));
  message.headers.insert(etl::make_pair("Upgrade", "websocket"));
  message.headers.insert(etl::make_pair("Connection", "Upgrade"));
  message.headers.insert(etl::make_pair("Sec-WebSocket-Version", "13"));
  message.headers.insert(
      etl::make_pair("User-Agent", "husarnet-daemon/" HUSARNET_VERSION));

  // Generate random nonce
  etl::array<uint8_t, 16> rawNonce;
  randombytes_buf(static_cast<void*>(rawNonce.data()), rawNonce.size());

  sodium_bin2base64(
      this->nonce.data(), this->nonce.size(), rawNonce.data(), rawNonce.size(),
      sodium_base64_VARIANT_ORIGINAL);

  HTTPMessage::HeaderMap::mapped_type value(
      this->nonce.begin(), this->nonce.end());
  message.headers.insert(etl::make_pair("Sec-WebSocket-Key", value));

  // Send handshake to server
  WebSocket::TransportBuffer buffer;
  message.encode(buffer);
  this->_sendRaw(buffer);

  return true;
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

void WebSocket::_sendRaw(etl::ivector<char>& data)
{
  assert(this->fd >= 0);
  ssize_t bytes_written = 0;

  // Send data supporting partial writes
  do {
    ssize_t n = SOCKFUNC(send)(this->fd, data.data(), data.size(), 0);

    if(n < 0) {
      LOG_ERROR("WS send failed: %s", strerror(errno));
      return;
    }

    bytes_written += n;
  } while(bytes_written != data.size());
}

void WebSocket::_readRaw()
{
  assert(this->fd >= 0);

  WebSocket::TransportBuffer buffer;

  // Read data into buffer memory
  // Buffer must be shrunk from the maximum size to the actual read size
  // to prevent reinitialization of the buffer underlying data type
  buffer.uninitialized_resize(buffer.capacity());
  ssize_t read = SOCKFUNC(recv)(this->fd, buffer.data(), buffer.capacity(), 0);
  buffer.resize(read);

  if(read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    LOG_WARNING("WS recv timeout");
    return;
  }

  if(read <= 0) {
    LOG_ERROR("WS recv failed: %s", strerror(errno));
    this->close();
    return;
  }

  LOG_INFO("WS received %d bytes", read);
  this->_handleRead(buffer);
}

// TODO: this is a temporary solution, should be replaced with a proper
// integration with sockets.cpp event loop (add TCP socket to the select set)
void WebSocket::_select(int timeout)
{
  // Skip if not connected
  if(this->fd < 0) {
    return;
  }

  fd_set readset;
  fd_set writeset;
  fd_set errorset;
  FD_ZERO(&writeset);
  FD_ZERO(&readset);
  FD_ZERO(&errorset);

  FD_SET(this->fd, &readset);
  FD_SET(this->fd, &errorset);

  struct timeval timeoutval;
  timeoutval.tv_sec = timeout / 1000;
  timeoutval.tv_usec = (timeout % 1000) * 1000;

  errno = 0;

  SOCKFUNC(select)(this->fd + 1, &readset, &writeset, &errorset, &timeoutval);

  if(FD_ISSET(this->fd, &readset) || FD_ISSET(this->fd, &errorset)) {
    this->_readRaw();
  }
}

void WebSocket::periodic(int timeout)
{
  // Handle incoming data
  this->_select(timeout);
}
