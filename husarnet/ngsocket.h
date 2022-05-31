// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <map>
#include <vector>
#include "enum.h"
#include "husarnet/device_id.h"
#include "husarnet/fstring.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/licensing.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/ngsocket_interfaces.h"
#include "husarnet/ngsocket_messages.h"
#include "husarnet/ngsocket_options.h"
#include "husarnet/peer_container.h"
#include "husarnet/ports/port.h"
#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/sockets.h"
#include "husarnet/queue.h"

class HusarnetManager;

const int REFRESH_TIMEOUT = 25 * 1000;
const int NAT_INIT_TIMEOUT = 3 * 1000;
const int TCP_PONG_TIMEOUT = 35 * 1000;
const int UDP_BASE_TIMEOUT = 35 * 1000;
const int REESTABLISH_TIMEOUT = 3 * 1000;
const int MAX_FAILED_ESTABLISHMENTS = 5;
const int MAX_ADDRESSES = 10;
const int MAX_SOURCE_ADDRESSES = 5;
const int DEVICEID_LENGTH = 16;
const int MULTICAST_PORT = 5581;

BETTER_ENUM(BaseConnectionType, int, NONE = 0, TCP = 1, UDP = 2)

class NgSocket : public LowerLayer {
 private:
  Identity* identity;
  HusarnetManager* manager;
  PeerContainer* peerContainer;

  DeviceId deviceId;
  fstring<32> pubkey;

  std::unordered_map<InetAddress, Peer*, iphash> peerSourceAddresses;
  std::vector<InetAddress> localAddresses;  // sorted
  Time lastRefresh = 0;
  Time lastPeriodic = 0;
  uint64_t natInitCounter = 0;
  uint64_t tcpCounter = 0;
  std::string cookie;

  Time lastBaseTcpMessage = 0;
  Time lastBaseTcpAction = 0;

  int baseConnectRetries = 0;
  InetAddress baseAddress;

  // Transient base addresses.
  // We send a packet to one port from a range of base ports (around 20 ports),
  // so we get new NAT mapping every time. (20 * 25s = 500s and UDP NAT mappings
  // typically last 30s-180s). This exists to get around permanently broken NAT
  // mapping (which may occur due to misconfiguration or Linux NAT
  // implementation not always assigning the same outbound port)
  int baseTransientPort = 0;
  std::pair<int, int> baseTransientRange = {0, 0};

  Time lastNatInitConfirmation = 0;
  Time lastNatInitSent = 0;

  bool natInitConfirmed = true;
  int queuedPackets = 0;

  int sourcePort = 0;

  InetAddress baseUdpAddress;
  std::vector<InetAddress> allBaseUdpAddresses;
  std::shared_ptr<OsSocket::FramedTcpConnection> baseConnection;

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
  void sendMulticast();
  void multicastPacketReceived(InetAddress address, string_view packetView);
  void init();
  void resendInfoRequests();
  void sendInfoRequestToBase(DeviceId id);
  std::string sign(const std::string& data, const std::string& kind);
  bool reloadLocalAddresses();
  void removeSourceAddress(Peer* peer, InetAddress address);
  void addSourceAddress(Peer* peer, InetAddress source);
  Peer* findPeerBySourceAddress(InetAddress address);
  std::vector<Peer*> iteratePeers();
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
  NgSocketOptions* options = new NgSocketOptions;

  NgSocket(HusarnetManager* manager);

  virtual void onUpperLayerData(DeviceId target, string_view data);
  void periodic();

  BaseConnectionType getCurrentBaseConnectionType();
  InetAddress getCurrentBaseAddress();

  int getLatency(DeviceId peer);
};
