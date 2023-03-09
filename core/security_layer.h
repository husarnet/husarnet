// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
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

  void handleHeartbeat(PeerId source, fstring<8> ident);
  void handleHeartbeatReply(PeerId source, fstring<8> ident);

  void handleDataPacket(PeerId source, string_view data);

  void sendHelloPacket(Peer* peer, int num = 1, uint64_t helloseq = 0);

  void handleHelloPacket(PeerId target, string_view data, int helloNum);
  void finishNegotiation(Peer* peer);

  void doSendDataPacket(Peer* peer, string_view data);

 public:
  SecurityLayer(HusarnetManager* manager);

  void onUpperLayerData(PeerId peerId, string_view data) override;
  void onLowerLayerData(PeerId peerId, string_view data) override;

  int getLatency(PeerId peerId);
};
