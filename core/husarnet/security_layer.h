// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/peer_container.h"
#include "husarnet/peer_flags.h"
#include "husarnet/string_view.h"

const uint64_t BOOT_ID_MASK = 0xFFFFFFFF00000000ull;

class SecurityLayer : public BidirectionalLayer {
 private:
  Identity* myIdentity;
  PeerFlags* myFlags;
  PeerContainer* peerContainer;

  std::string decryptedBuffer;
  std::string ciphertextBuffer;
  std::string cleartextBuffer;

  uint64_t helloseq = 0;

  int queuedPackets = 0;

  void handleHeartbeat(HusarnetAddress source, fstring<8> ident);
  void handleHeartbeatReply(HusarnetAddress source, fstring<8> ident);

  void handleDataPacket(HusarnetAddress source, string_view data);

  void sendHelloPacket(Peer* peer, int num = 1, uint64_t helloseq = 0);

  void
  handleHelloPacket(HusarnetAddress target, string_view data, int helloNum);
  void finishNegotiation(Peer* peer);

  void doSendDataPacket(Peer* peer, string_view data);

 public:
  SecurityLayer(
      Identity* myIdentity,
      PeerFlags* myFlags,
      PeerContainer* peerContainer);

  void onUpperLayerData(HusarnetAddress peerAddress, string_view data) override;
  void onLowerLayerData(HusarnetAddress peerAddress, string_view data) override;

  int getLatency(HusarnetAddress peerAddress);
};
