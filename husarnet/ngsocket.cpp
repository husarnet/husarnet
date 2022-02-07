// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)
#include "ngsocket.h"
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "global_lock.h"
#include "husarnet_config.h"
#include "husarnet_crypto.h"
#include "husarnet_manager.h"
#include "ports/port.h"
#include "queue.h"
#include "util.h"

const int TEARDOWN_TIMEOUT = 120 * 1000;
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
#if defined(ESP_PLATFORM)
const int WORKER_QUEUE_SIZE = 16;
#else
const int WORKER_QUEUE_SIZE = 256;
#endif

using Time = int64_t;
using mutex_guard = std::lock_guard<std::recursive_mutex>;

using namespace OsSocket;
using namespace NgSocketCrypto;

struct BaseToPeerMessage {
  enum { HELLO, DEVICE_ADDRESSES, DATA, NAT_OK, STATE, REDIRECT, INVALID } kind;

  // Hello message
  fstring<16> cookie;

  // State message
  std::vector<InetAddress> udpAddress;
  std::pair<int, int> natTransientRange;

  // Device addresses message
  DeviceId deviceId;
  std::vector<InetAddress> addresses;

  // Data message
  DeviceId source;
  std::string data;

  // Redirect
  InetAddress newBaseAddress;
};

struct PeerToBaseMessage {
  enum {
    REQUEST_INFO,
    DATA,
    INFO,
    NAT_INIT,
    USER_AGENT,
    NAT_OK_CONFIRM,
    NAT_INIT_TRANSIENT,
    INVALID
  } kind;

  // All
  fstring<16> cookie;

  // Request info
  DeviceId deviceId;

  // My info
  std::vector<InetAddress> addresses;

  // Data message
  DeviceId target;
  std::string data;

  // NAT init message
  uint64_t counter = 0;

  // User agent
  std::string userAgent;
};

struct PeerToPeerMessage {
  enum { HELLO, HELLO_REPLY, DATA, INVALID } kind;

  // hello and hello_reply
  DeviceId myId;
  DeviceId yourId;
  std::string helloCookie;

  // data message
  string_view data;
};

struct Peer {
  DeviceId id;
  Time lastPacket = 0;
  Time lastReestablish = 0;

  int latency = -1;  // in ms
  bool connected = false;
  bool reestablishing = false;
  int failedEstablishments = 0;
  InetAddress targetAddress;
  std::string helloCookie;

  std::vector<InetAddress> targetAddresses;
  InetAddress linkLocalAddress;
  std::unordered_set<InetAddress, iphash> sourceAddresses;
  std::vector<std::string> packetQueue;

  bool active() { return currentTime() - lastPacket < TEARDOWN_TIMEOUT; }
};

std::string idstr(DeviceId id) {
  return IpAddress::fromBinary(id).str();
}

#define IDSTR(id) idstr(id).c_str()

struct NgSocketImpl : public NgSocket {
  BaseConfig* baseConfig;
  std::unordered_map<DeviceId, Peer*> peers;
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
  IpAddress baseAddressFromDns;

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

  DeviceId deviceId;
  fstring<32> pubkey;
  Identity* identity;
  int sourcePort = 0;

  InetAddress baseUdpAddress;
  std::vector<InetAddress> allBaseUdpAddresses;
  std::shared_ptr<FramedTcpConnection> baseConnection;

  Queue<std::function<void()>> workerQueue;

  void periodic() override {
    if (currentTime() < lastPeriodic + 1000)
      return;
    lastPeriodic = currentTime();

    if (reloadLocalAddresses()) {
      // new addresses, accelerate reconnection
      LOG("IP address change detected");
      requestRefresh();
      if (currentTime() - lastBaseTcpAction > NAT_INIT_TIMEOUT)
        connectToBase();
    }

    if (currentTime() - lastRefresh > REFRESH_TIMEOUT)
      requestRefresh();

    if (!natInitConfirmed &&
        currentTime() - lastNatInitSent > NAT_INIT_TIMEOUT) {
      // retry nat init in case the packet gets lost
      sendNatInitToBase();
    }

    if (currentTime() - lastBaseTcpAction >
        (baseConnection ? TCP_PONG_TIMEOUT : NAT_INIT_TIMEOUT))
      connectToBase();
  }

  void resolveBaseAddress() {
    while (!baseAddressFromDns) {
      baseAddressFromDns = GIL::unlocked<IpAddress>(
          [&]() { return resolveToIp(baseConfig->getBaseDnsAddress()); });
      if (baseAddressFromDns)
        LOG("base address resolved to: %s", baseAddressFromDns.str().c_str());
      GIL::unlocked<void>([&]() { sleep(5); });
    }

    while (true) {
      IpAddress result = GIL::unlocked<IpAddress>(
          [&]() { return resolveToIp(baseConfig->getBaseDnsAddress()); });
      LOG("base address resolved to: %s", result.str().c_str());
      if (result)
        baseAddressFromDns = result;
      GIL::unlocked<void>([&]() { sleep(180); });
    }
  }

  std::string generalInfo(std::map<std::string, std::string> hosts =
                              std::map<std::string, std::string>()) override {
    std::string info;
    info += "Version: " HUSARNET_VERSION "\n";
    info +=
        "Husarnet IP address: " + IpAddress::fromBinary(deviceId).str() + "\n";
    if (hosts.find(IpAddress::fromBinary(deviceId).str()) != hosts.end()) {
      info += "Known hostnames: " +
              hosts.at(IpAddress::fromBinary(deviceId).str()) + "\n";
    }

    if (!isBaseUdp()) {
      if (currentTime() - lastBaseTcpMessage > UDP_BASE_TIMEOUT)
        info += "ERROR: no base connection\n";
      else
        info += "WARN: only TCP connection to base established\n";
    } else {
      info += "UDP connection to base: " + baseUdpAddress.str() + "\n";
    }

    return info;
  }

  std::string peerInfo(DeviceId id) override {
    Peer* peer = getPeerById(id);
    if (!peer)
      return "";
    if (!peer->active() && !peer->reestablishing)
      return "";

    std::string info;

    if (peer->sourceAddresses.size() > 0) {
      info += "  source=";
      for (InetAddress addr : peer->sourceAddresses) {
        info += addr.str() + " ";
      }
      info += "\n";
    }
    if (peer->targetAddresses.size() > 0) {
      info += "  addresses from base=";
      for (InetAddress addr : peer->targetAddresses) {
        info += addr.str() + " ";
      }
      info += "\n";
    }

    if (peer->targetAddress && peer->connected)
      info += "  target=" + peer->targetAddress.str() + "\n";
    else
      info += "  tunnelled\n";

    if (peer->linkLocalAddress && peer->connected)
      info += "  link local address=" + peer->linkLocalAddress.str() + "\n";

    return info;
  }

  int getLatency(DeviceId peerId) override { return -1; }

  void requestRefresh() {
    if (workerQueue.qsize() < WORKER_QUEUE_SIZE) {
      workerQueue.push(std::bind(&NgSocketImpl::refresh, this));
    } else {
      LOG("worker queue full");
    }
  }

  void workerLoop() {
    while (true) {
      std::function<void()> f = workerQueue.pop_blocking();
      GIL::lock();
      f();
      GIL::unlock();
    }
  }

  void refresh() {
    // release the lock around every operation to reduce latency
    lastRefresh = currentTime();
    sendLocalAddressesToBase();

    GIL::yield();

    sendNatInitToBase();
    sendMulticast();

    for (Peer* peer : iteratePeers()) {
      GIL::yield();
      periodicPeer(peer);
    }
  }

  void periodicPeer(Peer* peer) {
    if (!peer->active()) {
      peer->connected = false;
      // TODO: send unsubscribe to base
    } else {
      if (peer->reestablishing && peer->connected &&
          currentTime() - peer->lastReestablish > REESTABLISH_TIMEOUT) {
        peer->connected = false;
        LOG("falling back to relay (peer: %s)", IDSTR(peer->id));
      }

      attemptReestablish(peer);
    }
  }

  void sendDataPacket(DeviceId target, string_view data) override {
    HPERF_RECORD(husarnet_enter);
    Peer* peer = getOrCreatePeer(target);
    if (peer != nullptr)
      sendDataToPeer(peer, data);
  }

  bool isBaseUdp() {
    return currentTime() - lastNatInitConfirmation < UDP_BASE_TIMEOUT;
  }

  void sendDataToPeer(Peer* peer, string_view data) {
    if (peer->connected) {
      PeerToPeerMessage msg;
      msg.kind = PeerToPeerMessage::DATA;
      msg.data = data;
      HPERF_RECORD(husarnet_exit);

      LOG_DEBUG("send to peer %s", peer->targetAddress.str().c_str());
      sendToPeer(peer->targetAddress, msg);
    } else {
      if (!peer->reestablishing ||
          (currentTime() - peer->lastReestablish > REESTABLISH_TIMEOUT &&
           peer->failedEstablishments <= MAX_FAILED_ESTABLISHMENTS))
        attemptReestablish(peer);

      LOG_DEBUG("send to peer tunnelled");
      // Not (yet) connected, relay via base.
      PeerToBaseMessage msg;
      msg.target = peer->id;
      msg.kind = PeerToBaseMessage::DATA;
      msg.data = data;

      if (isBaseUdp() && !options->disableUdpTunnelling)
        sendToBaseUdp(msg);
      else if (!options->disableTcpTunnelling)
        sendToBaseTcp(msg);
    }

    if (!peer->active())
      sendInfoRequestToBase(peer->id);

    peer->lastPacket = currentTime();
  }

  Peer* createPeer(DeviceId id) {
    if (id == deviceId)
      return nullptr;  // self

    if (!options->isPeerAllowed(id)) {
      LOG("peer %s is not on the whitelist", IDSTR(id));
      return nullptr;
    }

    Peer* peer = new Peer;
    *peer = Peer{};
    peer->id = id;
    peers[id] = peer;
    LOG("createPeer %s", IDSTR(id));
    return peer;
  }

  void attemptReestablish(Peer* peer) {
    // TODO: if (peer->reestablishing) something;
    if (options->disableUdp)
      return;

    peer->failedEstablishments++;
    peer->lastReestablish = currentTime();
    peer->reestablishing = true;
    peer->helloCookie = randBytes(16);

    LOGV("reestablish connection to [%s]",
         IpAddress::fromBinary(peer->id).str().c_str());

    std::vector<InetAddress> addresses = peer->targetAddresses;
    if (peer->linkLocalAddress)
      addresses.push_back(peer->linkLocalAddress);

    for (InetAddress address : peer->sourceAddresses)
      addresses.push_back(address);

    for (InetAddress address : options->additionalPeerIpAddresses(peer->id))
      addresses.push_back(address);

    std::sort(addresses.begin(), addresses.end());
    addresses.erase(std::unique(addresses.begin(), addresses.end()),
                    addresses.end());

    std::string msg = "";

    for (InetAddress address : addresses) {
      if (address.ip.isFC94())
        continue;
      if (!options->isAddressAllowed(address))
        continue;
      msg += address.str().c_str();
      msg += ", ";
      PeerToPeerMessage response;
      response.kind = PeerToPeerMessage::HELLO;
      response.helloCookie = peer->helloCookie;
      response.yourId = peer->id;
      sendToPeer(address, response);

      if (address == peer->targetAddress)
        // send the heartbeat twice to the active address
        sendToPeer(address, response);
    }
    LOGV("addresses: %s", msg.c_str());
  }

  void peerMessageReceived(InetAddress source, const PeerToPeerMessage& msg) {
    if (msg.kind == PeerToPeerMessage::DATA) {
      peerDataPacketReceived(source, msg.data);
    } else if (msg.kind == PeerToPeerMessage::HELLO) {
      helloReceived(source, msg);
    } else if (msg.kind == PeerToPeerMessage::HELLO_REPLY) {
      helloReplyReceived(source, msg);
    }
  }

  void helloReceived(InetAddress source, const PeerToPeerMessage& msg) {
    if (msg.yourId != deviceId)
      return;

    Peer* peer = getOrCreatePeer(msg.myId);
    if (peer == nullptr)
      return;
    LOGV("HELLO from %s (id: %s, active: %s)", source.str().c_str(),
         IDSTR(msg.myId), peer->active() ? "YES" : "NO");

    if (!options->isAddressAllowed(source)) {
      LOGV("- rejected due to blacklist");
      return;
    }
    addSourceAddress(peer, source);

    if (source.ip.isLinkLocal() && !peer->linkLocalAddress) {
      peer->linkLocalAddress = source;
      if (peer->active())
        attemptReestablish(peer);
    }

    PeerToPeerMessage reply;
    reply.kind = PeerToPeerMessage::HELLO_REPLY;
    reply.helloCookie = msg.helloCookie;
    reply.yourId = msg.myId;
    sendToPeer(source, reply);
  }

  void helloReplyReceived(InetAddress source, const PeerToPeerMessage& msg) {
    if (msg.yourId != deviceId)
      return;

    LOGV("HELLO-REPLY from %s (%s)", source.str().c_str(),
         encodeHex(msg.myId).c_str());
    Peer* peer = getPeerById(msg.myId);
    if (peer == nullptr)
      return;
    if (!peer->reestablishing)
      return;
    if (peer->helloCookie != msg.helloCookie)
      return;

    if (!options->isAddressAllowed(source)) {
      LOGV("(address blacklisted)");
      return;
    }

    int latency = currentTime() - peer->lastReestablish;
    LOGV(" - using this address as target (reply received after %d ms)",
         latency);
    peer->targetAddress = source;
    peer->connected = true;
    peer->failedEstablishments = 0;
    peer->reestablishing = false;
  }

  void peerDataPacketReceived(InetAddress source, string_view data) {
    Peer* peer = findPeerBySourceAddress(source);

    if (peer != nullptr)
      delegate->onDataPacket(peer->id, data);
    else {
      LOG("unknown UDP data packet (source: %s)", source.str().c_str());
    }
  }

  void baseMessageReceivedUdp(const BaseToPeerMessage& msg) {
    if (msg.kind == BaseToPeerMessage::NAT_OK) {
      if (lastNatInitConfirmation == 0)
        LOG("UDP connection to base server established");
      lastNatInitConfirmation = currentTime();
      natInitConfirmed = true;

      PeerToBaseMessage resp{};
      resp.kind = PeerToBaseMessage::NAT_OK_CONFIRM;
      sendToBaseUdp(resp);
    } else if (msg.kind == BaseToPeerMessage::DATA) {
      delegate->onDataPacket(msg.source, msg.data);
    } else {
      LOG("received invalid UDP message from base");
    }
  }

  void baseMessageReceivedTcp(const BaseToPeerMessage& msg) {
    lastBaseTcpAction = lastBaseTcpMessage = currentTime();
    baseConnectRetries = 0;

    if (msg.kind == BaseToPeerMessage::HELLO) {
      LOG("TCP connection to base server established");
      LOGV("received hello cookie %s", encodeHex(msg.cookie).c_str());
      cookie = msg.cookie;
      resendInfoRequests();
      sendLocalAddressesToBase();
      requestRefresh();
    } else if (msg.kind == BaseToPeerMessage::DEVICE_ADDRESSES) {
      if (!options->disableUdp) {
        Peer* peer = getPeerById(msg.deviceId);
        if (peer != nullptr)
          changePeerTargetAddresses(peer, msg.addresses);
      }
    } else if (msg.kind == BaseToPeerMessage::DATA) {
      delegate->onDataPacket(msg.source, msg.data);
    } else if (msg.kind == BaseToPeerMessage::STATE) {
      if (!options->disableUdp) {
        allBaseUdpAddresses = msg.udpAddress;
        baseUdpAddress = msg.udpAddress[0];
        LOGV("received base UDP address: %s", baseUdpAddress.str().c_str());

        if (msg.natTransientRange.first != 0 &&
            msg.natTransientRange.second >= msg.natTransientRange.first) {
          LOGV("received base transient range: %d %d",
               msg.natTransientRange.first, msg.natTransientRange.second);
          baseTransientRange = msg.natTransientRange;
          if (baseTransientPort == 0) {
            baseTransientPort = baseTransientRange.first;
          }
        }
      }
    } else if (msg.kind == BaseToPeerMessage::REDIRECT) {
      baseAddress = msg.newBaseAddress;
      LOG("redirected to new base server: %s", baseAddress.str().c_str());
      connectToBase();
    } else {
      LOG("received invalid message from base (kind: %d)", msg.kind);
    }
  }

  void changePeerTargetAddresses(Peer* peer,
                                 std::vector<InetAddress> addresses) {
    std::sort(addresses.begin(), addresses.end());
    if (peer->targetAddresses != addresses) {
      peer->targetAddresses = addresses;
      attemptReestablish(peer);
    }
  }

  void sendNatInitToBase() {
    if (options->disableUdp)
      return;
    if (cookie.size() == 0)
      return;

    lastNatInitSent = currentTime();
    natInitConfirmed = false;

    PeerToBaseMessage msg;
    msg.kind = PeerToBaseMessage::NAT_INIT;
    msg.deviceId = deviceId;
    msg.counter = natInitCounter;
    natInitCounter++;

    sendToBaseUdp(msg);

    if (baseTransientPort != 0) {
      PeerToBaseMessage msg2;
      msg2.kind = PeerToBaseMessage::NAT_INIT_TRANSIENT;
      msg2.deviceId = deviceId;
      msg2.counter = natInitCounter;
      sendToBaseUdp(msg2);

      baseTransientPort++;
      if (baseTransientPort > baseTransientRange.second) {
        baseTransientPort = baseTransientRange.first;
      }
    }
  }

  void sendLocalAddressesToBase() {
    PeerToBaseMessage msg;
    msg.kind = PeerToBaseMessage::INFO;
    if (!options->disableUdp)
      msg.addresses = localAddresses;
    sendToBaseTcp(msg);
  }

  void sendMulticast() {
    if (options->disableUdp || options->disableMulticast)
      return;
    udpSendMulticast(InetAddress{MULTICAST_ADDR_4, MULTICAST_PORT},
                     pack((uint16_t)sourcePort) + deviceId);
    udpSendMulticast(InetAddress{MULTICAST_ADDR_6, MULTICAST_PORT},
                     pack((uint16_t)sourcePort) + deviceId);
  }

  void multicastPacketReceived(InetAddress address, string_view packetView) {
    // we check if this is from local network to avoid some DoS attacks
    if (!(address.ip.isLinkLocal() || address.ip.isPrivateV4()))
      return;

    if (options->disableUdp || options->disableMulticast)
      return;

    if (packetView.size() < DEVICEID_LENGTH + 2)
      return;

    std::string packet = packetView;

    DeviceId devId = packet.substr(2, DEVICEID_LENGTH);
    uint16_t port = unpack<uint16_t>(packet.substr(0, 2));

    if (devId == deviceId)
      return;

    Peer* peer = getPeerById(devId);

    LOGV("multicast received from %s, id %s, port %d, interesting: %s",
         address.str().c_str(), IDSTR(devId), port,
         peer == NULL ? "NO" : "YES");

    InetAddress srcAddress = InetAddress{address.ip, port};
    if (peer != nullptr && peer->linkLocalAddress != srcAddress) {
      peer->linkLocalAddress = srcAddress;
      attemptReestablish(peer);
    }
  }

  void init() {
    assert(pubkeyToDeviceId(pubkey) == deviceId);

    auto cb = [this](InetAddress address, string_view packet) {
      udpPacketReceived(address, packet);
    };

    sourcePort = 5582;
    if (options->overrideSourcePort)
      sourcePort = options->overrideSourcePort;

    for (;; sourcePort++) {
      if (sourcePort == 7000) {
        LOG("failed to bind UDP port");
        abort();
      }
      if (udpListenUnicast(sourcePort, cb))
        break;
    }

    auto callback = [this](InetAddress address, const std::string& packet) {
      multicastPacketReceived(address, packet);
    };
    udpListenMulticast(InetAddress{MULTICAST_ADDR_4, MULTICAST_PORT}, callback);
    udpListenMulticast(InetAddress{MULTICAST_ADDR_6, MULTICAST_PORT}, callback);

    // Passing std::bind crashes mingw_thread. The reason is not apparent.
    startThread([this]() { this->workerLoop(); }, "hworker",
                /*stack=*/8000);
    GIL::startThread([this]() { this->resolveBaseAddress(); }, "resolver",
                     /*stack=*/3000);

    LOG("ngsocket %s listening on %d", IDSTR(deviceId), sourcePort);
  }

  void resendInfoRequests() {
    for (Peer* peer : iteratePeers()) {
      if (peer->active())
        sendInfoRequestToBase(peer->id);
    }
  }

  void sendInfoRequestToBase(DeviceId id) {
    LOGV("info request %s", IDSTR(id));
    PeerToBaseMessage msg;
    msg.kind = PeerToBaseMessage::REQUEST_INFO;
    msg.deviceId = id;
    sendToBaseTcp(msg);
  }

  // communication functions
  void udpPacketReceived(InetAddress source, string_view data);
  void connectToBase();
  void sendToBaseUdp(const PeerToBaseMessage& msg);
  void sendToBaseTcp(const PeerToBaseMessage& msg);
  void sendToPeer(InetAddress dest, const PeerToPeerMessage& msg);

  // binary format
  std::string serializePeerToBaseMessage(const PeerToBaseMessage& msg);
  std::string serializePeerToPeerMessage(const PeerToPeerMessage& msg);

  // crypto
  std::string sign(const std::string& data, const std::string& kind) {
    return NgSocketCrypto::sign(data, kind, identity);
  }

  // data structure manipulation

  Peer* cachedPeer = nullptr;
  DeviceId cachedPeerId;

  Peer* getPeerById(DeviceId id) {
    if (cachedPeerId == id)
      return cachedPeer;

    auto it = peers.find(id);
    if (it == peers.end())
      return nullptr;

    cachedPeerId = id;
    cachedPeer = it->second;

    return it->second;
  }

  Peer* getOrCreatePeer(DeviceId id) {
    Peer* peer = getPeerById(id);
    if (peer == nullptr) {
      peer = createPeer(id);
    }
    return peer;
  }

  bool reloadLocalAddresses() {
    std::vector<InetAddress> newAddresses;
    if (options->useOverrideLocalAddresses) {
      newAddresses = options->overrideLocalAddresses;
    } else {
      for (IpAddress address : getLocalAddresses()) {
        if (address.data[0] == 0xfc && address.data[1] == 0x94)
          continue;

        newAddresses.push_back(InetAddress{address, (uint16_t)sourcePort});
      }
    }

    sort(newAddresses.begin(), newAddresses.end());
    if (newAddresses != localAddresses) {
      bool anyAddresses = localAddresses.size() > 0;
      localAddresses = newAddresses;
      return anyAddresses;
    } else {
      return false;
    }
  }

  void removeSourceAddress(Peer* peer, InetAddress address) {
    peer->sourceAddresses.erase(address);
    peerSourceAddresses.erase(address);
  }

  void addSourceAddress(Peer* peer, InetAddress source) {
    assert(peer != nullptr);
    if (peer->sourceAddresses.find(source) == peer->sourceAddresses.end()) {
      if (peer->sourceAddresses.size() >= MAX_SOURCE_ADDRESSES) {
        // remove random old address
        std::vector<InetAddress> addresses(peer->sourceAddresses.begin(),
                                           peer->sourceAddresses.end());
        removeSourceAddress(peer, addresses[rand() % addresses.size()]);
      }
      peer->sourceAddresses.insert(source);

      if (peerSourceAddresses.find(source) != peerSourceAddresses.end()) {
        Peer* lastPeer = peerSourceAddresses[source];
        lastPeer->sourceAddresses.erase(source);
      }
      peerSourceAddresses[source] = peer;
    }
  }

  Peer* findPeerBySourceAddress(InetAddress address) {
    auto it = peerSourceAddresses.find(address);
    if (it == peerSourceAddresses.end())
      return nullptr;
    else
      return it->second;
  }

  std::vector<Peer*> iteratePeers() {
    std::vector<Peer*> peersVec;
    for (auto& p : peers)
      peersVec.push_back(p.second);
    return peersVec;
  }
};

NgSocket* NgSocket::create(Identity* identity, BaseConfig* baseConfig) {
  NgSocketImpl* sock = new NgSocketImpl;
  sock->baseConfig = baseConfig;
  sock->pubkey = identity->pubkey;
  sock->deviceId = identity->deviceId;
  sock->identity = identity;
  sock->init();
  return sock;
}

BaseToPeerMessage parseBaseToPeerMessage(string_view data) {
  BaseToPeerMessage msg{};
  msg.kind = BaseToPeerMessage::INVALID;

  if (data.size() == 0)
    return msg;

  if (data[0] == (char)BaseToPeerMessage::HELLO) {
    if (data.size() != 17)
      return msg;
    msg.cookie = substr<1, 16>(data);
  } else if (data[0] == (char)BaseToPeerMessage::DEVICE_ADDRESSES) {
    if (data.size() <= 17)
      return msg;
    msg.deviceId = std::string(substr<1, 16>(data));
    for (int i = 17; i + 18 <= data.size(); i += 18) {
      msg.addresses.push_back(
          InetAddress{IpAddress::fromBinary(data.substr(i, 16)),
                      unpack<uint16_t>(data.substr(i + 16, 2))});
    }
  } else if (data[0] == (char)BaseToPeerMessage::DATA) {
    if (data.size() <= 17)
      return msg;
    msg.source = std::string(substr<1, 16>(data));
    msg.data = data.substr(17);
  } else if (data[0] == (char)BaseToPeerMessage::NAT_OK) {
  } else if (data[0] == (char)BaseToPeerMessage::STATE) {
    int i = 1;

    while (i + 18 <= data.size() && msg.udpAddress.size() < 5) {
      msg.udpAddress.push_back(
          InetAddress{IpAddress::fromBinary(data.substr(i, 16)),
                      unpack<uint16_t>(data.substr(i + 16, 2))});
      i += 18;
    }

    if (i + 4 <= data.size()) {
      msg.natTransientRange = {
          unpack<uint16_t>(data.substr(i, 2)),
          unpack<uint16_t>(data.substr(i + 2, 2)),
      };
    }

    if (msg.udpAddress.size() < 1)
      return msg;
  } else {
    return msg;
  }

  msg.kind = (decltype(msg.kind))data[0];
  return msg;
}

PeerToPeerMessage parsePeerToPeerMessage(string_view data) {
  PeerToPeerMessage msg{};
  msg.kind = PeerToPeerMessage::INVALID;
  if (data.size() == 0)
    return msg;

  if (data[0] == (char)PeerToPeerMessage::HELLO ||
      data[0] == (char)PeerToPeerMessage::HELLO_REPLY) {
    if (data.size() != 1 + 16 * 3 + 32 + 64)
      return msg;
    msg.myId = substr<1, 16>(data);
    std::string pubkey = data.substr(17, 32);
    msg.yourId = substr<17 + 32, 16>(data);
    msg.helloCookie = data.substr(17 + 48, 16);
    std::string signature = data.substr(17 + 64, 64);

    if (pubkeyToDeviceId(pubkey) != msg.myId) {
      LOG("invalid pubkey");
      return msg;
    }

    bool ok = GIL::unlocked<bool>([&]() {
      return verifySignature(data.substr(0, 17 + 64), "ng-p2p-msg", pubkey,
                             signature);
    });

    if (!ok) {
      LOG("invalid signature");
      return msg;
    }
    msg.kind = decltype(msg.kind)(data[0]);
    return msg;
  }

  if (data[0] == (char)PeerToPeerMessage::DATA) {
    msg.kind = decltype(msg.kind)(data[0]);
    msg.data = data.substr(1);
    return msg;
  }

  return msg;
}

std::string NgSocketImpl::serializePeerToPeerMessage(
    const PeerToPeerMessage& msg) {
  std::string data;

  if (msg.kind == PeerToPeerMessage::HELLO ||
      msg.kind == PeerToPeerMessage::HELLO_REPLY) {
    assert(deviceId.size() == 16 && msg.yourId.size() == 16 &&
           msg.helloCookie.size() == 16 && pubkey.size() == 32);
    data = pack((uint8_t)msg.kind) + deviceId + pubkey + msg.yourId +
           msg.helloCookie;
    data +=
        GIL::unlocked<std::string>([&]() { return sign(data, "ng-p2p-msg"); });
  } else if (msg.kind == PeerToPeerMessage::DATA) {
    data = pack((uint8_t)msg.kind) + msg.data.str();
  } else {
    abort();
  }

  return data;
}

std::string NgSocketImpl::serializePeerToBaseMessage(
    const PeerToBaseMessage& msg) {
  assert(deviceId.size() == 16 && cookie.size() == 16 && pubkey.size() == 32);
  std::string data =
      pack((uint8_t)msg.kind) + this->deviceId + this->pubkey + this->cookie;

  if (msg.kind == PeerToBaseMessage::REQUEST_INFO) {
    data += fstring<16>(msg.deviceId);
  } else if (msg.kind == PeerToBaseMessage::DATA) {
    data = pack((uint8_t)msg.kind) + this->deviceId;
    data += fstring<16>(msg.target);
    data += msg.data;
  } else if (msg.kind == PeerToBaseMessage::USER_AGENT) {
    return pack((uint8_t)msg.kind) + msg.userAgent;
  } else if (msg.kind == PeerToBaseMessage::INFO) {
    for (InetAddress address : localAddresses) {
      data += address.ip.toBinary() + pack((uint16_t)address.port);
    }
  } else if (msg.kind == PeerToBaseMessage::NAT_INIT) {
    data += pack((uint64_t)natInitCounter);
  } else if (msg.kind == PeerToBaseMessage::NAT_OK_CONFIRM) {
    data += pack((uint64_t)natInitCounter);
  }

  if (msg.kind != PeerToBaseMessage::DATA)
    data += sign(data, "ng-p2b-msg");

  return data;
}

void NgSocketImpl::udpPacketReceived(InetAddress source, string_view data) {
  LOG_DEBUG("udp received %s", source.str().c_str());
  if (source == baseUdpAddress) {
    baseMessageReceivedUdp(parseBaseToPeerMessage(data));
  } else {
    if (data[0] == (char)PeerToPeerMessage::HELLO ||
        data[0] == (char)PeerToPeerMessage::HELLO_REPLY) {
      // "slow" messages are handled on the worker thread to reduce latency
      if (workerQueue.qsize() < WORKER_QUEUE_SIZE) {
        std::string dataCopy = data;
        workerQueue.push([this, source, dataCopy]() {
          peerMessageReceived(source, parsePeerToPeerMessage(dataCopy));
        });
      } else {
        LOG("worker queue full");
      }
    } else {
      peerMessageReceived(source, parsePeerToPeerMessage(data));
    }
  }
}

void NgSocketImpl::connectToBase() {
  if (options->overrideBaseAddress) {
    baseAddress = options->overrideBaseAddress;
  } else {
    if (!baseAddress) {
      baseAddress = {baseAddressFromDns, 443};
    }

    if (baseConnectRetries > 2) {
      // failover
      auto baseTcpAddressesTmp = baseConfig->getBaseTcpAddresses();
      baseAddress =
          baseTcpAddressesTmp[baseConnectRetries % baseTcpAddressesTmp.size()];
      LOG("retrying with fallback base address: %s", baseAddress.str().c_str());
    }
  }
  baseConnectRetries++;
  if (!baseAddress) {
    LOG("waiting until base address is resolved...");
    return;
  }

  LOG("establishing connection to base %s", baseAddress.str().c_str());

  auto dataCallback = [this](string_view data) {
    lastBaseTcpMessage = currentTime();
    this->baseMessageReceivedTcp(parseBaseToPeerMessage(data));
  };

  auto errorCallback = [this](std::shared_ptr<FramedTcpConnection> conn) {
    LOG("base TCP connection closed");

    if (conn == baseConnection) {
      close(baseConnection);
      baseConnection = nullptr;  // warning: this will destroy us (the lambda)
      lastBaseTcpAction = currentTime();
    }
  };

  if (baseConnection)
    close(baseConnection);

  baseConnection = tcpConnect(baseAddress, dataCallback, errorCallback);
  lastBaseTcpAction = currentTime();

  PeerToBaseMessage userAgentMsg;
  userAgentMsg.kind = PeerToBaseMessage::USER_AGENT;
  userAgentMsg.userAgent = "Husarnet " HUSARNET_VERSION "\n";

#ifdef __linux__
  userAgentMsg.userAgent += "linux\n";
#elif ESP_PLATFORM
  userAgentMsg.userAgent += "esp32\n";
#else
  userAgentMsg.userAgent += "unknown os\n";
#endif

  userAgentMsg.userAgent += options->userAgent;
  sendToBaseTcp(userAgentMsg);
}

void NgSocketImpl::sendToBaseUdp(const PeerToBaseMessage& msg) {
  if (!baseConnection || cookie.size() == 0)
    return;
  std::string serialized = serializePeerToBaseMessage(msg);
  if (msg.kind == PeerToBaseMessage::NAT_INIT ||
      msg.kind == PeerToBaseMessage::NAT_OK_CONFIRM ||
      msg.kind == PeerToBaseMessage::NAT_INIT_TRANSIENT) {
    // In case of NatInit we send to all addresses, so base will know our
    // (maybe NATted) IPv6 address.
    for (InetAddress address : allBaseUdpAddresses) {
      if (msg.kind == PeerToBaseMessage::NAT_INIT_TRANSIENT) {
        address.port = baseTransientPort;
      } else {
        udpSend(address, serialized);
      }
    }
  } else {
    udpSend(baseUdpAddress, serialized);
  }
}

void NgSocketImpl::sendToBaseTcp(const PeerToBaseMessage& msg) {
  if (!baseConnection || cookie.size() == 0)
    return;
  std::string serialized = serializePeerToBaseMessage(msg);
  write(baseConnection, serialized, msg.kind != PeerToBaseMessage::DATA);
}

void NgSocketImpl::sendToPeer(InetAddress dest, const PeerToPeerMessage& msg) {
  std::string serialized = serializePeerToPeerMessage(msg);
  udpSend(dest, std::move(serialized));
}
