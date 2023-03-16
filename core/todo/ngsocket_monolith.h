// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

namespace OsSocket {
  struct FramedTcpConnection;
}  // namespace OsSocket

const int REFRESH_TIMEOUT = 25 * 1000;
const int NAT_INIT_TIMEOUT = 3 * 1000;
const int TCP_PONG_TIMEOUT = 35 * 1000;
const int UDP_BASE_TIMEOUT = 35 * 1000;
const int REESTABLISH_TIMEOUT = 3 * 1000;
const int MAX_FAILED_ESTABLISHMENTS = 5;
const int MAX_ADDRESSES = 10;
const int MAX_SOURCE_ADDRESSES = 5;

class NgSocket : public LowerLayer {
 private:
  // TODO change this to a queue of enums
  Queue<std::function<void()>> workerQueue;

  void requestRefresh();
  void workerLoop();
  void refresh();
  void periodicPeer(Peer* peer);
  bool isBaseUdp();
  void sendDataToPeer(Peer* peer, string_view data);
  void attemptReestablish(Peer* peer);
  void peerMessageReceived(InetAddress source, const PeerToPeerMessage& msg);
  void helloReceived(InetAddress source, const PeerToPeerMessage& msg);
  void helloReplyReceived(InetAddress source, const PeerToPeerMessage& msg);
  void peerDataPacketReceived(InetAddress source, string_view data);
  void baseMessageReceivedUdp(const BaseToPeerMessage& msg);
  void baseMessageReceivedTcp(const BaseToPeerMessage& msg);
  void changePeerTargetAddresses(
      Peer* peer,
      std::vector<InetAddress> addresses);
  void sendNatInitToBase();
  void sendLocalAddressesToBase();
  void sendMulticast();  // yeah, that's definitely not a good place for that
  void multicastPacketReceived(
      InetAddress address,
      string_view packetView);  // same
  void init();
  void resendInfoRequests();
  void sendInfoRequestToBase(PeerId id);
  std::string sign(
      const std::string& data,
      const std::string& kind);  // why… why it's in this layer…
  bool reloadLocalAddresses();   // i think this should be moved to the
                                 // HusarnetManager
  void removeSourceAddress(Peer* peer, InetAddress address);
  void addSourceAddress(Peer* peer, InetAddress source);
  Peer* findPeerBySourceAddress(
      InetAddress address);           // move it to PeerContainer
  std::vector<Peer*> iteratePeers();  // PeerContainer
  BaseToPeerMessage parseBaseToPeerMessage(string_view data);
  PeerToPeerMessage parsePeerToPeerMessage(string_view data);
  std::string serializePeerToPeerMessage(const PeerToPeerMessage& msg);
  std::string serializePeerToBaseMessage(const PeerToBaseMessage& msg);

  void udpPacketReceived(InetAddress source, string_view data);
  void connectToBase();
  void sendToBaseUdp(const PeerToBaseMessage& msg);
  void sendToBaseTcp(const PeerToBaseMessage& msg);
  void sendToPeer(InetAddress dest, const PeerToPeerMessage& msg);

 public:
  NgSocket(HusarnetManager* manager);

  virtual void onUpperLayerData(PeerId peerId, string_view data);
  void periodic();
};
