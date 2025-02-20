// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/security_layer.h"

#include <sodium.h>

#include "husarnet/ports/port.h"
#include "husarnet/ports/port_interface.h"

#include "husarnet/husarnet_config.h"
#include "husarnet/logging.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/util.h"

static fstring<32> mixFlags(fstring<32> key, uint64_t flags1, uint64_t flags2)
{
  fstring<32> res;
  std::string k = std::string(key) + pack(flags1) + pack(flags2);
  crypto_hash_sha256(&res[0], (const unsigned char*)&k[0], k.size());
  return res;
}

SecurityLayer::SecurityLayer(
    Identity* myIdentity,
    PeerFlags* myFlags,
    PeerContainer* peerContainer)
    : myIdentity(myIdentity), myFlags(myFlags), peerContainer(peerContainer)
{
  randombytes_buf(&this->helloseq, 8);
  this->helloseq = this->helloseq & BOOT_ID_MASK;
  this->decryptedBuffer.resize(2000);
  this->ciphertextBuffer.resize(2100);
  this->cleartextBuffer.resize(2010);
}

int SecurityLayer::getLatency(HusarnetAddress peerId)
{
  Peer* peer = peerContainer->getOrCreatePeer(peerId);
  if(peer == nullptr) {
    return -1;
  }

  peer->heartbeatIdent = generateRandomString(8);
  std::string packet = std::string("\4") + peer->heartbeatIdent;
  peer->lastLatencySent = Port::getCurrentTime();
  sendToLowerLayer(peer->id, packet);

  if(peer->lastLatencyReceived + 10000 < Port::getCurrentTime()) {
    return -1;
  }

  return peer->latency;
}

void SecurityLayer::handleHeartbeat(HusarnetAddress source, fstring<8> ident)
{
  std::string packet = std::string("\5") + ident;
  sendToLowerLayer(source, packet);
}

void SecurityLayer::handleHeartbeatReply(
    HusarnetAddress source,
    fstring<8> ident)
{
  Peer* peer = peerContainer->getOrCreatePeer(source);
  if(peer == nullptr)
    return;

  if(ident == peer->heartbeatIdent) {
    Time now = Port::getCurrentTime();
    peer->lastLatencyReceived = now;
    peer->latency = now - peer->lastLatencySent;
  }
}

void SecurityLayer::onLowerLayerData(HusarnetAddress peerId, string_view data)
{
  if(data.size() >= decryptedBuffer.size())
    return;  // sanity check

  // TODO Make a proper serializer/deserializer for this part of the protocol
  if(data[0] == 0) {  // data packet
    if(data.size() <= 25)
      return;
    handleDataPacket(peerId, data);
  } else if(data[0] == 1 || data[0] == 2 || data[0] == 3) {  // hello packet
    if(data.size() <= 25)
      return;

    handleHelloPacket(peerId, data, (int)data[0]);
  } else if(data[0] == 4 || data[0] == 5) {  // heartbeat (hopefully they are
                                             // not cursed)
    if(data.size() < 9)
      return;

    std::string ident = substr<1, 8>(data);
    if(data[0] == 4) {
      handleHeartbeat(peerId, ident);
    } else {
      handleHeartbeatReply(peerId, ident);
    }
  }
}

void SecurityLayer::handleDataPacket(HusarnetAddress peerId, string_view data)
{
  LOG_DEBUG("received data packet from peer: %s", peerId.toString().c_str())
  const int headerSize = 1 + 24 + 16;
  if(data.size() <= headerSize + 8)
    return;

  Peer* peer = peerContainer->getOrCreatePeer(peerId);  // TODO long term - DoS
  if(peer == nullptr)
    return;

  if(!peer->negotiated) {
    sendHelloPacket(peer);
    LOG_WARNING(
        "received data packet before hello from peer: %s",
        peerId.toString().c_str());
    return;
  }

  int decryptedSize = int(data.size()) - headerSize;
  if(decryptedBuffer.size() < decryptedSize)
    return;

  int r = crypto_secretbox_open_easy(
      (unsigned char*)&decryptedBuffer[0],
      (unsigned char*)&data[25],  // ciphertext
      data.size() - 25,
      (unsigned char*)&data[1],  // nonce
      peer->rxKey.data());
  if(decryptedSize <= 8)
    return;

  auto decryptedData =
      string_view(decryptedBuffer).substr(8, decryptedSize - 8);

  if(r == 0) {
    sendToUpperLayer(peerId, decryptedData);
  } else {
    LOG_INFO(
        "received forged message from peer: %s", peerId.toString().c_str());
  }
}

void SecurityLayer::sendHelloPacket(Peer* peer, int num, uint64_t helloseq)
{
  assert(num == 1 || num == 2 || num == 3);
  std::string packet;
  packet.push_back((char)num);
  packet += this->myIdentity->getPubkey();
  packet += peer->kxPubkey;
  packet += peer->id;
  packet += pack(this->helloseq);
  packet += pack(helloseq);
  packet += pack(this->myFlags->asBin());
  packet += NgSocketCrypto::sign(packet, "ng-kx-pubkey", this->myIdentity);
  sendToLowerLayer(peer->id, packet);
}

void SecurityLayer::handleHelloPacket(
    HusarnetAddress target,
    string_view data,
    int helloNum)
{
  constexpr int dataLen = 65 + 16 + 16;
  if(data.size() < dataLen + 64)
    return;
  LOG_INFO("hello %d from %s", helloNum, target.toString().c_str());

  Peer* peer = peerContainer->getOrCreatePeer(target);
  if(peer == nullptr)
    return;

  fstring<32> incomingPubkey = substr<1, 32>(data);
  fstring<32> peerKxPubkey = substr<33, 32>(data);
  fstring<16> targetId = substr<65, 16>(data);
  uint64_t yourHelloseq = unpack<uint64_t>(substr<65 + 16, 8>(data));
  uint64_t myHelloseq = unpack<uint64_t>(substr<65 + 24, 8>(data));
  fstring<64> signature = data.substr(data.size() - 64).str();
  uint64_t flags_bin = 0;
  if(data.size() >= 64 + 65 + 32 + 8) {
    flags_bin = unpack<uint64_t>(substr<65 + 32, 8>(data));
  }
  LOG_DEBUG(
      "peer flags: %llx (bits, not count)", (unsigned long long)flags_bin);

  if(targetId != this->myIdentity->getDeviceId()) {
    LOG_INFO(
        "misdirected hello packet received from peer: %s",
        std::string(targetId).c_str());
    return;
  }

  if(NgSocketCrypto::pubkeyToDeviceId(incomingPubkey) != target) {
    LOG_INFO(
        "forged hello packet received (invalid pubkey: %s)",
        std::string(incomingPubkey).c_str());
    return;
  }

  if(!NgSocketCrypto::verifySignature(
         data.substr(0, data.size() - 64), "ng-kx-pubkey", incomingPubkey,
         signature)) {
    LOG_CRITICAL(
        "forged hello packet (invalid signature: %s)",
        std::string(signature).c_str());
    return;
  }

  if(helloNum == 1) {
    sendHelloPacket(peer, 2, yourHelloseq);
    return;
  }

  if(myHelloseq != this->helloseq) {  // prevents replay DoS
    // this will occur under normal operation, if both sides attempt to
    // initialize at once or two handshakes are interleaved
    LOG_DEBUG("invalid helloseq from %s", peer->getIpAddressString().c_str());
    return;
  }

  peer->flags = PeerFlags(flags_bin);

  int r;
  // key exchange is asymmetric, pretend that device with smaller ID is a
  // client
  if(target < this->myIdentity->getDeviceId())
    r = crypto_kx_client_session_keys(
        peer->rxKey.data(), peer->txKey.data(), peer->kxPubkey.data(),
        peer->kxPrivkey.data(), peerKxPubkey.data());
  else
    r = crypto_kx_server_session_keys(
        peer->rxKey.data(), peer->txKey.data(), peer->kxPubkey.data(),
        peer->kxPrivkey.data(), peerKxPubkey.data());

  if(flags_bin != 0) {
    // we need to make sure both peers agree on flags - mix them into the key
    // exchange
    peer->rxKey = mixFlags(peer->rxKey, flags_bin, this->myFlags->asBin());
    peer->txKey = mixFlags(peer->txKey, this->myFlags->asBin(), flags_bin);
  }

  if(r == 0) {
    LOG_DEBUG(
        "negotiated session keys %s %s",
        encodeHex(peer->rxKey.substr(0, 6)).c_str(),
        encodeHex(peer->txKey.substr(0, 6)).c_str());
    finishNegotiation(peer);
  } else {
    LOG_INFO("key exchange failed with %s", peer->getIpAddressString().c_str());
    return;
  }

  this->helloseq++;

  if(helloNum == 2)
    sendHelloPacket(peer, 3, yourHelloseq);
}

void SecurityLayer::finishNegotiation(Peer* peer)
{
  LOG_INFO(
      "established secure connection to %s",
      peer->getIpAddressString().c_str());
  peer->negotiated = true;
  for(auto& packet : peer->packetQueue) {
    queuedPackets--;
    doSendDataPacket(peer, packet);
  }
  peer->packetQueue = std::vector<std::string>();
}

void SecurityLayer::onUpperLayerData(HusarnetAddress target, string_view data)
{
  Peer* peer = peerContainer->getOrCreatePeer(target);
  if(peer == nullptr)
    return;
  if(peer->negotiated) {
    doSendDataPacket(peer, data);
  } else {
    if(queuedPackets < MAX_QUEUED_PACKETS) {
      queuedPackets++;
      peer->packetQueue.push_back(data);
    }
    sendHelloPacket(peer);
  }
}

void SecurityLayer::doSendDataPacket(Peer* peer, string_view data)
{
  uint64_t seqnum = 0;
  assert(data.size() < 10240);
  int cleartextSize = (int)data.size() + 8;

  if(data.size() >= cleartextBuffer.size())
    return;
  if(cleartextSize >= cleartextBuffer.size())
    return;

  packTo(seqnum, &cleartextBuffer[0]);
  memcpy(&cleartextBuffer[8], data.data(), data.size());

  int ciphertextSize = 1 + 24 + 16 + cleartextSize;
  ciphertextBuffer[0] = 0;

  if(ciphertextSize >= ciphertextBuffer.size())
    return;

  char* nonce = &ciphertextBuffer[1];
  randombytes_buf(nonce, 24);

  crypto_secretbox_easy(
      (unsigned char*)&ciphertextBuffer[25],
      (const unsigned char*)cleartextBuffer.data(), cleartextSize,
      (const unsigned char*)nonce, peer->txKey.data());

  sendToLowerLayer(
      peer->id, string_view(ciphertextBuffer).substr(0, ciphertextSize));
}
