// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/ngsocket.h"
#include "husarnet/peer_container.h"

const uint64_t BOOT_ID_MASK = 0xFFFFFFFF00000000ull;

const uint64_t FLAG_SUPPORTS_FLAGS = 0x1;
const uint64_t FLAG_COMPRESSION = 0x2;

class SecurityLayer : public BidirectionalLayer {
 private:
  HusarnetManager* manager;

  std::string decryptedBuffer;
  std::string ciphertextBuffer;
  std::string cleartextBuffer;
  std::string compressBuffer;

  uint64_t helloseq = 0;

  int queuedPackets = 0;

  void handleHeartbeat(DeviceId source, fstring<8> ident);
  void handleHeartbeatReply(DeviceId source, fstring<8> ident);

  void handleDataPacket(DeviceId source, string_view data);

  void sendHelloPacket(Peer* peer, int num = 1, uint64_t helloseq = 0);
  uint64_t getMyFlags();

  void handleHelloPacket(DeviceId target, string_view data, int helloNum);
  void finishNegotiation(Peer* peer);

  void doSendDataPacket(Peer* peer, string_view data);

  PeerContainer* peerContainer;

 public:
  SecurityLayer(HusarnetManager* manager);

  void onUpperLayerData(DeviceId source, string_view data) override;
  void onLowerLayerData(DeviceId target, string_view data) override;

  int getLatency(DeviceId peerId);
};
