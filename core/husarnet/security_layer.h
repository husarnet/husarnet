// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/ngsocket.h"
#include "husarnet/peer_container.h"

const uint64_t BOOT_ID_MASK = 0xFFFFFFFF00000000ull;

class SecurityLayer : public BidirectionalLayer {
 private:
  HusarnetManager* manager;

  std::string decryptedBuffer;
  std::string ciphertextBuffer;
  std::string cleartextBuffer;

  PeerContainer* peerContainer;

  uint64_t helloseq = 0;

  int queuedPackets = 0;

  void handleHeartbeat(DeviceId source, fstring<8> ident);
  void handleHeartbeatReply(DeviceId source, fstring<8> ident);

  void handleDataPacket(DeviceId source, string_view data);

  void sendHelloPacket(Peer* peer, int num = 1, uint64_t helloseq = 0);

  void handleHelloPacket(DeviceId target, string_view data, int helloNum);
  void finishNegotiation(Peer* peer);

  void doSendDataPacket(Peer* peer, string_view data);

 public:
  SecurityLayer(HusarnetManager* manager);

  void onUpperLayerData(DeviceId peerId, string_view data) override;
  void onLowerLayerData(DeviceId peerId, string_view data) override;

  int getLatency(DeviceId peerId);
};
