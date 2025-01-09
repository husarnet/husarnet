// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <etl/array.h>

#include "sodium.h"
#include "websocket.h"

size_t WebSocket::Message::parse(
    etl::string_view& buffer,
    WebSocket::Message& message)
{
  // Smallest possible message is 2 bytes long
  if(buffer.size() < 2)
    return 0;

  auto it = buffer.begin();

  char flags = *it++;
  message.flags.fin = flags & 0x80;
  message.flags.rsv1 = flags & 0x40;
  message.flags.rsv2 = flags & 0x20;
  message.flags.rsv3 = flags & 0x10;

  // Websocket extension specific flags
  // As we don't support any, they must be set to 0
  if(message.flags.rsv1 || message.flags.rsv2 || message.flags.rsv3)
    return 0;

  // Fragmented messages are not supported
  if(!message.flags.fin)
    return 0;

  message.opcode = static_cast<Message::Opcode>(flags & 0x0F);

  char byte = *it++;
  message.masked = byte & 0x80;
  char len_byte = byte & 0x7F;

  size_t length;
  // Length uses variable length encoding
  if(len_byte < 126) {
    length = len_byte;
  } else if(len_byte == 126 && std::distance(it, buffer.end()) >= 2) {
    length = *it++;
    length = (length << 8) | *it++;
  } else if(len_byte == 127 && std::distance(it, buffer.end()) >= 8) {
    length = *it++;
    length = (length << 8) | *it++;
    length = (length << 8) | *it++;
    length = (length << 8) | *it++;
    length = (length << 8) | *it++;
    length = (length << 8) | *it++;
    length = (length << 8) | *it++;
    length = (length << 8) | *it++;
  } else {
    // Message is too short
    return 0;
  }

  // Masking key is present only if message is masked
  etl::array<uint8_t, 4> maskingKey;
  if(message.masked) {
    if(std::distance(it, buffer.end()) < 4)
      return 0;

    maskingKey.assign(it, it + 4);
    it += 4;
  }

  // Copy message data
  // TODO: move/view instead of copy?
  if(std::distance(it, buffer.end()) < length)
    return 0;

  message.data.assign(it, it + length);
  it += length;

  // Unmask data if needed
  if(message.masked) {
    for(size_t i = 0; i < length; i++)
      message.data[i] ^= maskingKey[i % 4];
  }

  return std::distance(buffer.begin(), it);
}

bool WebSocket::Message::encode(etl::ivector<char>& buffer) const
{
  buffer.clear();

  char flags = 0;
  flags |= this->flags.fin << 7;
  flags |= this->flags.rsv1 << 6;
  flags |= this->flags.rsv2 << 5;
  flags |= this->flags.rsv3 << 4;
  flags |= static_cast<uint8_t>(this->opcode) & 0x0F;

  buffer.push_back(flags);
  uint64_t length = this->data.size();

  // Encode mask bit and length as a VLE
  if(length < 126) {
    buffer.push_back(length | this->masked << 7);
  } else if(length < 65536) {
    buffer.push_back(126 | masked << 7);
    buffer.push_back(length >> 8);
    buffer.push_back(length & 0xFF);
  } else {
    buffer.push_back(127 | masked << 7);
    buffer.push_back(length >> 56);
    buffer.push_back(length >> 48);
    buffer.push_back(length >> 40);
    buffer.push_back(length >> 32);
    buffer.push_back(length >> 24);
    buffer.push_back(length >> 16);
    buffer.push_back(length >> 8);
    buffer.push_back(length & 0xFF);
  }

  if(buffer.available() < length) {
    buffer.clear();
    return false;
  }

  if(length == 0) {
    return true;
  }

  if(this->masked) {
    // Generate random masking key per each message
    etl::array<uint8_t, 4> maskingKey;
    randombytes_buf(maskingKey.data(), maskingKey.size());

    // Masking key is present only if message is masked
    buffer.insert(buffer.end(), maskingKey.begin(), maskingKey.end());

    for(size_t i = 0; i < length; i++)
      buffer.push_back(this->data[i] ^ maskingKey[i % 4]);
  } else {
    buffer.insert(buffer.end(), this->data.begin(), this->data.end());
  }

  return true;
}
