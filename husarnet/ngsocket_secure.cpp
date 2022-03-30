// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "ngsocket_secure.h"
#include <sodium.h>
#include <unordered_map>
#include "ngsocket_crypto.h"
#include "port.h"
#include "util.h"
#ifdef WITH_ZSTD
#include "zstd.h"
#endif

const int MAX_QUEUED_PACKETS = 10;
const uint64_t BOOT_ID_MASK = 0xFFFFFFFF00000000ull;
using Time = int64_t;

const uint64_t FLAG_SUPPORTS_FLAGS = 0x1;
const uint64_t FLAG_COMPRESSION = 0x2;

using namespace NgSocketCrypto;

struct SecPeer {
  bool negotiated = false;
  DeviceId id;
  fstring<32> kxPubkey;
  fstring<32> kxPrivkey;

  fstring<32> txKey;
  fstring<32> rxKey;

  int latency = -1;
  Time lastLatencyReceived = 0;
  Time lastLatencySent = 0;
  fstring<8> heartbeatIdent;

  uint64_t flags;

  std::vector<std::string> packetQueue;
};

#define Peer SecPeer

fstring<32> mixFlags(fstring<32> key, uint64_t flags1, uint64_t flags2) {
  fstring<32> res;
  std::string k = std::string(key) + pack(flags1) + pack(flags2);
  crypto_hash_sha256(&res[0], (const unsigned char*)&k[0], k.size());
  return res;
}

struct NgSocketSecureImpl : public NgSocket, public NgSocketDelegate {
  NgSocket* socket = nullptr;
  fstring<32> pubkey;
  Identity* identity = nullptr;
  DeviceId deviceId;

  std::string decryptedBuffer;
  std::string ciphertextBuffer;
  std::string cleartextBuffer;
  std::string compressBuffer;

  uint64_t helloseq = 0;

  int queuedPackets = 0;

  std::unordered_map<DeviceId, Peer*> peers;

  NgSocketSecureImpl() {
    randombytes_buf(&helloseq, 8);
    helloseq = helloseq & BOOT_ID_MASK;
    decryptedBuffer.resize(2000);
    ciphertextBuffer.resize(2100);
    cleartextBuffer.resize(2010);
    compressBuffer.resize(2100);
  }

  std::string generalInfo(std::map<std::string, std::string> hosts =
                              std::map<std::string, std::string>()) override {
    return socket->generalInfo(hosts);
  }

  int getLatency(DeviceId peerId) override {
    Peer* peer = getOrCreatePeer(peerId);
    if (peer == nullptr)
      return -1;

    peer->heartbeatIdent = randBytes(8);
    std::string packet = std::string("\4") + peer->heartbeatIdent;
    peer->lastLatencySent = currentTime();
    socket->sendDataPacket(peer->id, packet);

    if (peer->lastLatencyReceived + 10000 < currentTime())
      return -1;
    return peer->latency;
  }

  void handleHeartbeat(DeviceId source, fstring<8> ident) {
    std::string packet = std::string("\5") + ident;
    socket->sendDataPacket(source, packet);
  }

  void handleHeartbeatReply(DeviceId source, fstring<8> ident) {
    Peer* peer = getOrCreatePeer(source);
    if (peer == nullptr)
      return;

    if (ident == peer->heartbeatIdent) {
      Time now = currentTime();
      peer->lastLatencyReceived = now;
      peer->latency = now - peer->lastLatencySent;
    }
  }

  std::string peerInfo(DeviceId id) override {
    std::string infostr = socket->peerInfo(id);
    Peer* peer = getPeer(id);
    if (peer) {
      if (peer->negotiated)
        infostr += "  secure connection established\n";
      else
        infostr += "  establishing secure connection\n";
    }
    return infostr;
  }

  std::string info(std::map<std::string, std::string> hosts =
                       std::map<std::string, std::string>()) override {
    std::string result = generalInfo(hosts);
    for (auto k : peers) {
      result += "Peer " + IpAddress::fromBinary(k.first).str();
      result += options->peerAddressInfo(k.first);
      if (hosts.find(IpAddress::fromBinary(k.first).str()) != hosts.end()) {
        result += "\n";
        result += "  Known hostnames: " +
                  hosts.at(IpAddress::fromBinary(k.first).str());
      }
      result += "\n";
      result += peerInfo(k.first);
    }
    return result;
  }

  void onDataPacket(DeviceId source, string_view data) override {
    if (data.size() >= decryptedBuffer.size())
      return;  // sanity check

    if (data[0] == 0) {  // data packet
      if (data.size() <= 25)
        return;
      handleDataPacket(source, data);
    } else if (data[0] == 1 || data[0] == 2 || data[0] == 3) {  // hello packet
      if (data.size() <= 25)
        return;
      handleHelloPacket(source, data, (int)data[0]);
    } else if (data[0] == 4 ||
               data[0] == 5) {  // heartbeat (hopefully they are not cursed)
      if (data.size() < 9)
        return;

      std::string ident = substr<1, 8>(data);
      if (data[0] == 4) {
        handleHeartbeat(source, ident);
      } else {
        handleHeartbeatReply(source, ident);
      }
    }
  }

  void handleDataPacket(DeviceId source, string_view data) {
    const int headerSize = 1 + 24 + 16;
    if (data.size() <= headerSize + 8)
      return;

    Peer* peer = getOrCreatePeer(source);  // TODO: DoS
    if (peer == nullptr)
      return;

    if (!peer->negotiated) {
      sendHelloPacket(peer);
      LOGV("received data packet before hello");
      return;
    }

    int decryptedSize = int(data.size()) - headerSize;
    if (decryptedBuffer.size() < decryptedSize)
      return;

    int r = crypto_secretbox_open_easy((unsigned char*)&decryptedBuffer[0],
                                       (unsigned char*)&data[25],  // ciphertext
                                       data.size() - 25,
                                       (unsigned char*)&data[1],  // nonce
                                       peer->rxKey.data());
    if (decryptedSize <= 8)
      return;

    string_view decryptedData;
    if (options->compressionEnabled && (peer->flags & FLAG_COMPRESSION)) {
#ifdef WITH_ZSTD
      size_t s = ZSTD_decompress(&compressBuffer[0], compressBuffer.size(),
                                 &decryptedBuffer[8], decryptedSize - 8);
      if (ZSTD_isError(s)) {
        LOG("ZSTD decompression failed");
        return;
      }
      decryptedData = string_view(compressBuffer).substr(0, s);
#else
      abort();
#endif
    } else {
      decryptedData = string_view(decryptedBuffer).substr(8, decryptedSize - 8);
    }

    // uint64_t seqnum = unpack<uint64_t>(decrypted.substr(0, 8));
    if (r == 0) {
      delegate->onDataPacket(source, decryptedData);
    } else {
      LOG("received forged message");
    }
  }

  void sendHelloPacket(Peer* peer, int num = 1, uint64_t helloseq = 0) {
    assert(num == 1 || num == 2 || num == 3);
    std::string packet;
    packet.push_back((char)num);
    packet += pubkey;
    packet += peer->kxPubkey;
    packet += peer->id;
    packet += pack(this->helloseq);
    packet += pack(helloseq);
    packet += pack(getMyFlags());
    packet += sign(packet, "ng-kx-pubkey", identity);
    socket->sendDataPacket(peer->id, packet);
  }

  uint64_t getMyFlags() {
    uint64_t flags = 0;
    flags |= FLAG_SUPPORTS_FLAGS;
#ifdef WITH_ZSTD
    if (options->compressionEnabled)
      flags |= FLAG_COMPRESSION;
#endif
    return flags;
  }

  void handleHelloPacket(DeviceId target, string_view data, int helloNum) {
    constexpr int dataLen = 65 + 16 + 16;
    if (data.size() < dataLen + 64)
      return;
    LOGV("hello %d from %s", helloNum, encodeHex(target).c_str());

    Peer* peer = getOrCreatePeer(target);
    if (peer == nullptr)
      return;

    fstring<32> incomingPubkey = substr<1, 32>(data);
    fstring<32> peerKxPubkey = substr<33, 32>(data);
    fstring<16> targetId = substr<65, 16>(data);
    uint64_t yourHelloseq = unpack<uint64_t>(substr<65 + 16, 8>(data));
    uint64_t myHelloseq = unpack<uint64_t>(substr<65 + 24, 8>(data));
    fstring<64> signature = data.substr(data.size() - 64).str();
    uint64_t flags = 0;
    if (data.size() >= 64 + 65 + 32 + 8) {
      flags = unpack<uint64_t>(substr<65 + 32, 8>(data));
    }
    LOGV("peer flags: %llx", (unsigned long long)flags);

    if (targetId != deviceId) {
      LOG("misdirected hello packet");
      return;
    }

    if (pubkeyToDeviceId(incomingPubkey) != target) {
      LOG("forged hello packet (invalid pubkey)");
      return;
    }

    if (!verifySignature(data.substr(0, data.size() - 64), "ng-kx-pubkey",
                         incomingPubkey, signature)) {
      LOG("forged hello packet (invalid signature)");
      return;
    }

    if (helloNum == 1) {
      sendHelloPacket(peer, 2, yourHelloseq);
      return;
    }

    if (myHelloseq != this->helloseq) {  // prevents replay DoS
      // this will occur under normal operation, if both sides attempt to
      // initialize at once or two handshakes are interleaved
      LOGV("invalid helloseq");
      return;
    }

    peer->flags = flags;

    int r;
    // key exchange is asymmetric, pretend that device with smaller ID is a
    // client
    if (target < deviceId)
      r = crypto_kx_client_session_keys(
          peer->rxKey.data(), peer->txKey.data(), peer->kxPubkey.data(),
          peer->kxPrivkey.data(), peerKxPubkey.data());
    else
      r = crypto_kx_server_session_keys(
          peer->rxKey.data(), peer->txKey.data(), peer->kxPubkey.data(),
          peer->kxPrivkey.data(), peerKxPubkey.data());

    if (flags != 0) {
      // we need to make sure both peers agree on flags - mix them into the key
      // exchange
      peer->rxKey = mixFlags(peer->rxKey, flags, getMyFlags());
      peer->txKey = mixFlags(peer->txKey, getMyFlags(), flags);
    }

    if (r == 0) {
      LOGV("negotiated session keys %s %s",
           encodeHex(peer->rxKey.substr(0, 6)).c_str(),
           encodeHex(peer->txKey.substr(0, 6)).c_str());
      finishNegotiation(peer);
    } else {
      LOG("key exchange failed");
      return;
    }

    this->helloseq++;

    if (helloNum == 2)
      sendHelloPacket(peer, 3, yourHelloseq);
  }

  void finishNegotiation(Peer* peer) {
    LOG("established secure connection to %s", encodeHex(peer->id).c_str());
    peer->negotiated = true;
    for (auto& packet : peer->packetQueue) {
      queuedPackets--;
      doSendDataPacket(peer, packet);
    }
    peer->packetQueue = std::vector<std::string>();
  }

  void sendDataPacket(DeviceId target, string_view data) override {
    Peer* peer = getOrCreatePeer(target);
    if (peer == nullptr)
      return;
    if (peer->negotiated) {
      doSendDataPacket(peer, data);
    } else {
      if (queuedPackets < MAX_QUEUED_PACKETS) {
        queuedPackets++;
        peer->packetQueue.push_back(data);
      }
      sendHelloPacket(peer);
    }
  }

  void doSendDataPacket(Peer* peer, string_view data) {
    HPERF_RECORD(secure_enter);
    uint64_t seqnum = 0;
    assert(data.size() < 10240);
    int cleartextSize = (int)data.size() + 8;

    if (data.size() >= cleartextBuffer.size())
      return;
    if (cleartextSize >= cleartextBuffer.size())
      return;

    packTo(seqnum, &cleartextBuffer[0]);
    if (options->compressionEnabled && (peer->flags & FLAG_COMPRESSION)) {
#ifdef WITH_ZSTD
      size_t s = ZSTD_compress(&cleartextBuffer[8], cleartextBuffer.size() - 8,
                               data.data(), data.size(), /*level=*/1);
      if (ZSTD_isError(s)) {
        LOG("ZSTD compression failed");
        return;
      }
      cleartextSize = (int)s + 8;
#else
      abort();
#endif
    } else {
      memcpy(&cleartextBuffer[8], data.data(), data.size());
    }

    int ciphertextSize = 1 + 24 + 16 + cleartextSize;
    ciphertextBuffer[0] = 0;

    if (ciphertextSize >= ciphertextBuffer.size())
      return;

    char* nonce = &ciphertextBuffer[1];
    randombytes_buf(nonce, 24);

    crypto_secretbox_easy((unsigned char*)&ciphertextBuffer[25],
                          (const unsigned char*)cleartextBuffer.data(),
                          cleartextSize, (const unsigned char*)nonce,
                          peer->txKey.data());

    HPERF_RECORD(secure_exit);
    socket->sendDataPacket(
        peer->id, string_view(ciphertextBuffer).substr(0, ciphertextSize));
  }

  void periodic() override { socket->periodic(); }

  Peer* createPeer(DeviceId id) {
    if (!options->isPeerAllowed(id)) {
      LOG("peer %s is not on the whitelist", encodeHex(id).c_str());
      return nullptr;
    }
    Peer* peer = new Peer;
    peer->heartbeatIdent = randBytes(8);
    peer->id = id;
    crypto_kx_keypair(peer->kxPubkey.data(), peer->kxPrivkey.data());
    peers[id] = peer;
    return peer;
  }

  Peer* cachedPeer = nullptr;
  DeviceId cachedPeerId;

  // data structure manipulation
  Peer* getPeer(DeviceId id) {
    if (cachedPeerId == id)
      return cachedPeer;

    if (!options->isPeerAllowed(id)) {
      LOG("peer %s is not on the whitelist", encodeHex(id).c_str());
      return nullptr;
    }

    if (id == deviceId)
      return nullptr;

    auto it = peers.find(id);
    if (it == peers.end())
      return nullptr;

    cachedPeerId = id;
    cachedPeer = it->second;
    return it->second;
  }

  Peer* getOrCreatePeer(DeviceId id) {
    Peer* peer = getPeer(id);
    if (peer == nullptr) {
      peer = createPeer(id);
    }
    return peer;
  }
};

NgSocket* NgSocketSecure::create(Identity* identity, HusarnetManager* manager) {
  NgSocketSecureImpl* self = new NgSocketSecureImpl;
  self->pubkey = identity->pubkey;
  self->deviceId = identity->deviceId;
  self->identity = identity;
  self->socket = NgSocket::create(identity, manager);
  self->options = self->socket->options;
  self->socket->delegate = self;
  return self;
}
