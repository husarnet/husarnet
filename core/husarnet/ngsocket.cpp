// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ngsocket.h"

#include <algorithm>
#include <array>
#include <mutex>
#include <unordered_map>

#include <assert.h>
#include <stdlib.h>

#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/fstring.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/logging.h"
#include "husarnet/ngsocket.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/util.h"

#if defined(ESP_PLATFORM)
const int WORKER_QUEUE_SIZE = 16;
#else
const int WORKER_QUEUE_SIZE = 256;
#endif

using namespace OsSocket;

NgSocket::NgSocket(Identity* myIdentity, PeerContainer* peerContainer, ConfigManager* configManager)
    : myIdentity(myIdentity), peerContainer(peerContainer), configManager(configManager)
{
  init();
}

void NgSocket::periodic()
{
  if(Port::getCurrentTime() < lastPeriodic + 1000)
    return;
  lastPeriodic = Port::getCurrentTime();

  if(reloadLocalAddresses()) {
    // new addresses, accelerate reconnection
    {
      std::string addresses;
      // Add separator between addresses
      for(auto iter = localAddresses.begin(); iter != localAddresses.end(); iter++) {
        if(iter != localAddresses.begin())
          addresses += " | ";
        addresses += iter->str();
      }

      LOG_INFO("Local IP address change detected, new addresses: %s", addresses.c_str());
    }
    requestRefresh();
    if(Port::getCurrentTime() - lastBaseTcpAction > NAT_INIT_TIMEOUT)
      connectToBase();
  }

  if(Port::getCurrentTime() - lastRefresh > REFRESH_TIMEOUT)
    requestRefresh();

  if(!natInitConfirmed && Port::getCurrentTime() - lastNatInitSent > NAT_INIT_TIMEOUT) {
    // retry nat init in case the packet gets lost
    sendNatInitToBase();
  }

  if(Port::getCurrentTime() - lastBaseTcpAction > (baseConnection ? TCP_PONG_TIMEOUT : NAT_INIT_TIMEOUT))
    connectToBase();
}

BaseConnectionType NgSocket::getCurrentBaseConnectionType()
{
  if(isBaseUdp()) {
    return BaseConnectionType::UDP;
  }

  if(Port::getCurrentTime() - lastBaseTcpMessage <= UDP_BASE_TIMEOUT) {
    return BaseConnectionType::TCP;
  }

  return BaseConnectionType::None;
};

InetAddress NgSocket::getCurrentBaseAddress()
{
  if(isBaseUdp()) {
    return baseUdpAddress;
  }

  return baseAddress;
};

void NgSocket::requestRefresh()
{
  if(workerQueue.qsize() < WORKER_QUEUE_SIZE) {
    workerQueue.push(std::bind(&NgSocket::refresh, this));
  } else {
    LOG_ERROR("ngsocket worker queue full");
  }
}

void NgSocket::workerLoop()
{
  while(true) {
    std::function<void()> f = workerQueue.pop_blocking();
    f();
  }
}

void NgSocket::refresh()
{
  lastRefresh = Port::getCurrentTime();
  sendLocalAddressesToBase();

  sendNatInitToBase();
  sendMulticast();

  for(Peer* peer : iteratePeers()) {
    periodicPeer(peer);
  }
}

void NgSocket::periodicPeer(Peer* peer)
{
  if(!peer->isActive()) {
    peer->connected = false;
    // TODO long term - send unsubscribe to base
  } else {
    if(peer->reestablishing && peer->connected &&
       Port::getCurrentTime() - peer->lastReestablish > REESTABLISH_TIMEOUT) {
      peer->connected = false;
      LOG_WARNING("falling back to relay (peer: %s)", peer->getIpAddressString().c_str());
    }

    attemptReestablish(peer);
  }
}

void NgSocket::onUpperLayerData(HusarnetAddress peerAddress, string_view data)
{
  Peer* peer = peerContainer->getOrCreatePeer(peerAddress);
  if(peer != nullptr)
    sendDataToPeer(peer, data);
}

bool NgSocket::isBaseUdp()
{
  return Port::getCurrentTime() - lastNatInitConfirmation < UDP_BASE_TIMEOUT;
}

void NgSocket::sendDataToPeer(Peer* peer, string_view data)
{
  if(peer->connected) {
    PeerToPeerMessage msg = {
        .kind = PeerToPeerMessageKind::DATA,
        .data = data,
    };

    LOG_DEBUG("send to peer %s (%d bytes)", peer->targetAddress.str().c_str(), data.size());
    sendToPeer(peer->targetAddress, msg);
  } else {
    if(!peer->reestablishing || (Port::getCurrentTime() - peer->lastReestablish > REESTABLISH_TIMEOUT &&
                                 peer->failedEstablishments <= MAX_FAILED_ESTABLISHMENTS))
      attemptReestablish(peer);

    LOG_DEBUG("send to peer %s tunnelled", peer->getIpAddressString().c_str());
    // Not (yet) connected, relay via base.
    PeerToBaseMessage msg = {
        .kind = PeerToBaseMessageKind::DATA,
        .target = peer->id.data,
        .data = data,
    };

    if(isBaseUdp()) {
      sendToBaseUdp(msg);
    } else {
      sendToBaseTcp(msg);
    }
  }

  if(!peer->isActive())
    sendInfoRequestToBase(peer->id);

  peer->lastPacket = Port::getCurrentTime();
}

void NgSocket::attemptReestablish(Peer* peer)
{
  // TODO long term - if (peer->reestablishing) something;
  peer->failedEstablishments++;
  peer->lastReestablish = Port::getCurrentTime();
  peer->reestablishing = true;
  peer->helloCookie = generateRandomString(16);

  LOG_INFO("reestablish connection to [%s]", peer->getIpAddressString().c_str());

  std::vector<InetAddress> addresses = peer->targetAddresses;
  if(peer->linkLocalAddress)
    addresses.push_back(peer->linkLocalAddress);

  for(InetAddress address : peer->sourceAddresses)
    addresses.push_back(address);

  std::sort(addresses.begin(), addresses.end());
  addresses.erase(std::unique(addresses.begin(), addresses.end()), addresses.end());

  std::string msg = "";

  for(InetAddress address : addresses) {
    if(address.ip.isFC94())
      continue;

    msg += address.str().c_str();
    msg += ", ";
    PeerToPeerMessage response = {
        .kind = PeerToPeerMessageKind::HELLO,
        .yourId = peer->id.data,
        .helloCookie = peer->helloCookie,
    };
    sendToPeer(address, response);

    if(address == peer->targetAddress)
      // send the heartbeat twice to the active address
      sendToPeer(address, response);
  }
  LOG_DEBUG("attempt reestablish %s addresses: %s", peer->getIpAddressString().c_str(), msg.c_str());
}

void NgSocket::peerMessageReceived(InetAddress source, const PeerToPeerMessage& msg)
{
  switch(msg.kind) {
    case +PeerToPeerMessageKind::DATA:
      peerDataPacketReceived(source, msg.data);
      break;
    case +PeerToPeerMessageKind::HELLO:
      helloReceived(source, msg);
      break;
    case +PeerToPeerMessageKind::HELLO_REPLY:
      helloReplyReceived(source, msg);
      break;
    default:
      LOG_ERROR("unknown message received from peer %s", source.str().c_str());
  }
}

void NgSocket::helloReceived(InetAddress source, const PeerToPeerMessage& msg)
{
  if(msg.yourId != this->myIdentity->getDeviceId().data) {
    return;
  }

  Peer* peer = peerContainer->getOrCreatePeer(msg.myId);
  if(peer == nullptr)
    return;
  LOG_DEBUG(
      "HELLO from %s (id: %s, active: %s)", source.str().c_str(), msg.myId.toString().c_str(),
      peer->isActive() ? "YES" : "NO");

  addSourceAddress(peer, source);

  if(source.ip.isLinkLocal() && !peer->linkLocalAddress) {
    peer->linkLocalAddress = source;
    if(peer->isActive())
      attemptReestablish(peer);
  }

  PeerToPeerMessage reply = {
      .kind = PeerToPeerMessageKind::HELLO_REPLY,
      .yourId = msg.myId,
      .helloCookie = msg.helloCookie,
  };
  sendToPeer(source, reply);
}

void NgSocket::helloReplyReceived(InetAddress source, const PeerToPeerMessage& msg)
{
  if(msg.yourId != this->myIdentity->getDeviceId().data) {
    return;
  }

  LOG_DEBUG("HELLO-REPLY from %s (%s)", source.str().c_str(), msg.myId.toString().c_str());
  Peer* peer = peerContainer->getPeer(msg.myId);
  if(peer == nullptr) {
    return;
  }
  if(!peer->reestablishing) {
    return;
  }
  if(peer->helloCookie != msg.helloCookie) {
    return;
  }

  int latency = Port::getCurrentTime() - peer->lastReestablish;
  LOG_DEBUG(" - using this address as target (reply received after %d ms)", latency);
  peer->targetAddress = source;
  peer->connected = true;
  peer->failedEstablishments = 0;
  peer->reestablishing = false;
}

void NgSocket::peerDataPacketReceived(InetAddress source, string_view data)
{
  Peer* peer = findPeerBySourceAddress(source);

  if(peer != nullptr)
    sendToUpperLayer(peer->id, data);
  else {
    LOG_ERROR("unknown UDP data packet (source: %s)", source.str().c_str());
  }
}

void NgSocket::baseMessageReceivedUdp(const BaseToPeerMessage& msg)
{
  switch(msg.kind) {
    case +BaseToPeerMessageKind::DATA:
      sendToUpperLayer(msg.source, msg.data);
      break;
    case +BaseToPeerMessageKind::NAT_OK: {
      if(lastNatInitConfirmation == 0) {
        LOG_INFO("UDP connection to base server established, server address: %s", baseAddress.str().c_str());
      }
      lastNatInitConfirmation = Port::getCurrentTime();
      natInitConfirmed = true;

      PeerToBaseMessage resp{
          .kind = PeerToBaseMessageKind::NAT_OK_CONFIRM,
      };
      sendToBaseUdp(resp);
    } break;
    default:
      LOG_ERROR("received invalid UDP message from base (kind: %s)", msg.kind._to_string());
  }
}

void NgSocket::baseMessageReceivedTcp(const BaseToPeerMessage& msg)
{
  lastBaseTcpAction = lastBaseTcpMessage = Port::getCurrentTime();
  baseConnectRetries = 0;

  Peer* peer = nullptr;

  switch(msg.kind) {
    case +BaseToPeerMessageKind::DATA:
      sendToUpperLayer(msg.source, msg.data);
      break;
    case +BaseToPeerMessageKind::HELLO:
      LOG_INFO("TCP connection to base server established, server address: %s", baseAddress.str().c_str());
      LOG_DEBUG("received hello cookie %s", encodeHex(msg.cookie).c_str());
      cookie = msg.cookie;
      resendInfoRequests();
      sendLocalAddressesToBase();
      requestRefresh();
      break;
    case +BaseToPeerMessageKind::DEVICE_ADDRESSES:
      peer = peerContainer->getPeer(msg.deviceId);
      if(peer != nullptr)
        changePeerTargetAddresses(peer, msg.addresses);
      break;
    case +BaseToPeerMessageKind::STATE:
      allBaseUdpAddresses = msg.udpAddress;
      baseUdpAddress = msg.udpAddress[0];
      LOG_DEBUG("received base UDP address: %s", baseUdpAddress.str().c_str());

      if(msg.natTransientRange.first != 0 && msg.natTransientRange.second >= msg.natTransientRange.first) {
        LOG_DEBUG("received base transient range: %d %d", msg.natTransientRange.first, msg.natTransientRange.second);
        baseTransientRange = msg.natTransientRange;
        if(baseTransientPort == 0) {
          baseTransientPort = baseTransientRange.first;
        }
      }
      break;
    case +BaseToPeerMessageKind::REDIRECT:
      baseAddress = msg.newBaseAddress;
      LOG_INFO("redirected to new base server: %s", baseAddress.str().c_str());
      connectToBase();
      break;
    default:
      LOG_ERROR("received invalid TCP message from base (kind: %s)", msg.kind._to_string());
  }
}

void NgSocket::changePeerTargetAddresses(Peer* peer, std::vector<InetAddress> addresses)
{
  std::sort(addresses.begin(), addresses.end());
  if(peer->targetAddresses != addresses) {
    peer->targetAddresses = addresses;
    attemptReestablish(peer);
  }
}

void NgSocket::sendNatInitToBase()
{
  if(cookie.size() == 0)
    return;

  lastNatInitSent = Port::getCurrentTime();
  natInitConfirmed = false;

  PeerToBaseMessage msg = {
      .kind = PeerToBaseMessageKind::NAT_INIT,
      .deviceId = this->myIdentity->getDeviceId().data,
      .counter = natInitCounter,
  };
  natInitCounter++;

  sendToBaseUdp(msg);

  if(baseTransientPort != 0) {
    PeerToBaseMessage msg2 = {
        .kind = PeerToBaseMessageKind::NAT_INIT_TRANSIENT,
        .deviceId = this->myIdentity->getDeviceId().data,
        .counter = natInitCounter,
    };
    sendToBaseUdp(msg2);

    baseTransientPort++;
    if(baseTransientPort > baseTransientRange.second) {
      baseTransientPort = baseTransientRange.first;
    }
  }
}

void NgSocket::sendLocalAddressesToBase()
{
  PeerToBaseMessage msg = {
      .kind = PeerToBaseMessageKind::INFO,
      .addresses = localAddresses,
  };
  sendToBaseTcp(msg);
}

void NgSocket::sendMulticast()
{
  udpSendMulticast(
      InetAddress{MULTICAST_ADDR_4, MULTICAST_PORT}, pack((uint16_t)sourcePort) + this->myIdentity->getDeviceId().data);
  udpSendMulticast(
      InetAddress{MULTICAST_ADDR_6, MULTICAST_PORT}, pack((uint16_t)sourcePort) + this->myIdentity->getDeviceId().data);
}

void NgSocket::multicastPacketReceived(InetAddress address, string_view packetView)
{
  // we check if this is from local network to avoid some DoS attacks
  if(!(address.ip.isLinkLocal() || address.ip.isPrivateV4()))
    return;

  if(packetView.size() < DEVICEID_LENGTH + 2)
    return;

  std::string packet = packetView;

  // TODO make this into a proper message type
  HusarnetAddress devId = HusarnetAddress::fromBinaryString(packet.substr(2, DEVICEID_LENGTH));
  uint16_t port = unpack<uint16_t>(packet.substr(0, 2));

  if(devId == this->myIdentity->getDeviceId())
    return;

  Peer* peer = peerContainer->getPeer(devId);

  LOG_DEBUG(
      "multicast received from %s, id %s, port %d, interesting: %s", address.str().c_str(), devId.toString().c_str(),
      port, peer == NULL ? "NO" : "YES");

  InetAddress srcAddress = InetAddress{address.ip, port};
  if(peer != nullptr && peer->linkLocalAddress != srcAddress) {
    peer->linkLocalAddress = srcAddress;
    attemptReestablish(peer);
  }
}

void NgSocket::init()
{
  assert(NgSocketCrypto::pubkeyToDeviceId(this->myIdentity->getPubkey()) == this->myIdentity->getDeviceId());

  auto cb = [this](InetAddress address, string_view packet) { udpPacketReceived(address, packet); };

  sourcePort = 5582;  // TODO make this a macro definition

  for(;; sourcePort++) {
    if(sourcePort == 7000) {  // TODO this too
      LOG_CRITICAL("failed to bind UDP port");
      abort();
    }
    if(udpListenUnicast(sourcePort, cb)) {
      break;
    }
  }

  auto callback = [this](InetAddress address, const std::string& packet) { multicastPacketReceived(address, packet); };
  udpListenMulticast(InetAddress{MULTICAST_ADDR_4, MULTICAST_PORT}, callback);
  udpListenMulticast(InetAddress{MULTICAST_ADDR_6, MULTICAST_PORT}, callback);

  // Passing std::bind crashes mingw_thread. The reason is not apparent.
  Port::threadStart(
      [this]() { this->workerLoop(); }, "hnet_ng",
      /*stack=*/8000,
      NGSOCKET_TASK_PRIORITY);  // TODO figure whether this is still useful

  LOG_INFO("ngsocket %s listening on %d", this->myIdentity->getDeviceId().toString().c_str(), sourcePort);
}

void NgSocket::resendInfoRequests()
{
  for(Peer* peer : iteratePeers()) {
    if(peer->isActive())
      sendInfoRequestToBase(peer->id);
  }
}

void NgSocket::sendInfoRequestToBase(HusarnetAddress id)
{
  LOG_DEBUG("info request %s", id.toString().c_str());
  PeerToBaseMessage msg = {
      .kind = PeerToBaseMessageKind::REQUEST_INFO,
      .deviceId = id.data,
  };
  sendToBaseTcp(msg);
}

// crypto
std::string NgSocket::sign(const std::string& data, const std::string& kind)
{
  return NgSocketCrypto::sign(data, kind, myIdentity);
}

// data structure manipulation

Peer* cachedPeer = nullptr;
HusarnetAddress cachedPeerId;

bool NgSocket::reloadLocalAddresses()
{
  std::vector<InetAddress> newAddresses;

  for(IpAddress address : Port::getLocalAddresses()) {
    if(address.isFC94()) {
      continue;
    }

    newAddresses.push_back(InetAddress{address, (uint16_t)sourcePort});
  }

  sort(newAddresses.begin(), newAddresses.end());
  if(newAddresses != localAddresses) {
    bool anyAddresses = localAddresses.size() > 0;
    localAddresses = newAddresses;
    return anyAddresses;
  } else {
    return false;
  }
}

void NgSocket::removeSourceAddress(Peer* peer, InetAddress address)
{
  peer->sourceAddresses.erase(address);
  peerSourceAddresses.erase(address);
}

void NgSocket::addSourceAddress(Peer* peer, InetAddress source)
{
  assert(peer != nullptr);
  if(peer->sourceAddresses.find(source) == peer->sourceAddresses.end()) {
    if(peer->sourceAddresses.size() >= MAX_SOURCE_ADDRESSES) {
      // remove random old address
      std::vector<InetAddress> addresses(peer->sourceAddresses.begin(), peer->sourceAddresses.end());
      removeSourceAddress(peer, addresses[rand() % addresses.size()]);
    }
    peer->sourceAddresses.insert(source);

    if(peerSourceAddresses.find(source) != peerSourceAddresses.end()) {
      Peer* lastPeer = peerSourceAddresses[source];
      lastPeer->sourceAddresses.erase(source);
    }
    peerSourceAddresses[source] = peer;
  }
}

Peer* NgSocket::findPeerBySourceAddress(InetAddress address)
{
  auto it = peerSourceAddresses.find(address);
  if(it == peerSourceAddresses.end())
    return nullptr;
  else
    return it->second;
}

std::vector<Peer*> NgSocket::iteratePeers()
{
  std::vector<Peer*> peersVec;
  for(auto& p : peerContainer->getPeers())
    peersVec.push_back(p.second);
  return peersVec;
}

BaseToPeerMessage NgSocket::parseBaseToPeerMessage(string_view data)
{
  BaseToPeerMessage msg{
      .kind = BaseToPeerMessageKind::INVALID,
  };

  if(data.size() == 0)
    return msg;

  if(data[0] == (char)BaseToPeerMessageKind::HELLO) {
    if(data.size() != 17)
      return msg;
    msg.cookie = substr<1, 16>(data);
  } else if(data[0] == (char)BaseToPeerMessageKind::DEVICE_ADDRESSES) {
    if(data.size() <= 17)
      return msg;
    msg.deviceId = substr<1, 16>(data);
    for(int i = 17; i + 18 <= data.size(); i += 18) {
      msg.addresses.push_back(
          InetAddress{IpAddress::fromBinaryString(data.substr(i, 16)), unpack<uint16_t>(data.substr(i + 16, 2))});
    }
  } else if(data[0] == (char)BaseToPeerMessageKind::DATA) {
    if(data.size() <= 17)
      return msg;
    msg.source = substr<1, 16>(data);
    msg.data = data.substr(17);
  } else if(data[0] == (char)BaseToPeerMessageKind::NAT_OK) {
  } else if(data[0] == (char)BaseToPeerMessageKind::STATE) {
    int i = 1;

    while(i + 18 <= data.size() && msg.udpAddress.size() < 5) {
      msg.udpAddress.push_back(
          InetAddress{IpAddress::fromBinaryString(data.substr(i, 16)), unpack<uint16_t>(data.substr(i + 16, 2))});
      i += 18;
    }

    if(i + 4 <= data.size()) {
      msg.natTransientRange = {
          unpack<uint16_t>(data.substr(i, 2)),
          unpack<uint16_t>(data.substr(i + 2, 2)),
      };
    }

    if(msg.udpAddress.size() < 1)
      return msg;
  } else {
    return msg;
  }

  // TODO: refactor, dead code?
  auto msgKind = BaseToPeerMessageKind::_from_integral_nothrow(data[0]);
  if(msgKind) {
    msg.kind = msgKind.value();
  } else {
    LOG_DEBUG("invalid message kind: %d", data[0]);
  }

  return msg;
}

PeerToPeerMessage NgSocket::parsePeerToPeerMessage(string_view data)
{
  PeerToPeerMessage msg = {
      .kind = PeerToPeerMessageKind::INVALID,
  };
  if(data.size() == 0)
    return msg;

  if(data[0] == (char)PeerToPeerMessageKind::HELLO || data[0] == (char)PeerToPeerMessageKind::HELLO_REPLY) {
    if(data.size() != 1 + 16 * 3 + 32 + 64)
      return msg;
    msg.myId = substr<1, 16>(data);
    std::string pubkey = data.substr(17, 32);
    msg.yourId = substr<17 + 32, 16>(data);
    msg.helloCookie = data.substr(17 + 48, 16);
    std::string signature = data.substr(17 + 64, 64);

    if(NgSocketCrypto::pubkeyToDeviceId(pubkey) != msg.myId) {
      LOG_ERROR("invalid pubkey: %s", pubkey.c_str());
      return msg;
    }

    bool ok = NgSocketCrypto::verifySignature(data.substr(0, 17 + 64), "ng-p2p-msg", pubkey, signature);

    if(!ok) {
      LOG_ERROR("invalid signature: %s", signature.c_str());
      return msg;
    }
    msg.kind = PeerToPeerMessageKind::_from_index_unchecked(data[0]);
    return msg;
  }

  if(data[0] == (char)PeerToPeerMessageKind::DATA) {
    msg.kind = PeerToPeerMessageKind::_from_integral_unchecked(data[0]);
    msg.data = data.substr(1);
    return msg;
  }

  return msg;
}

std::string NgSocket::serializePeerToPeerMessage(const PeerToPeerMessage& msg)
{
  std::string data;

  switch(msg.kind) {
    case +PeerToPeerMessageKind::HELLO:
    case +PeerToPeerMessageKind::HELLO_REPLY:
      assert(
          this->myIdentity->getDeviceId().data.size() == 16 && msg.yourId.data.size() == 16 &&
          msg.helloCookie.size() == 16 && this->myIdentity->getPubkey().size() == 32);
      data = pack((uint8_t)msg.kind._value) + this->myIdentity->getDeviceId().data + this->myIdentity->getPubkey() +
             msg.yourId.data + msg.helloCookie;
      data += sign(data, "ng-p2p-msg");
      break;
    case +PeerToPeerMessageKind::DATA:
      data = pack((uint8_t)msg.kind._value) + msg.data.str();
      break;
    default:
      abort();
  }

  return data;
}

std::string NgSocket::serializePeerToBaseMessage(const PeerToBaseMessage& msg)
{
  assert(
      this->myIdentity->getDeviceId().data.size() == 16 && cookie.size() == 16 &&
      this->myIdentity->getPubkey().size() == 32);

  std::string data = pack((uint8_t)msg.kind._value) + this->myIdentity->getDeviceId().data +
                     this->myIdentity->getPubkey() + this->cookie;

  switch(msg.kind) {
    case +PeerToBaseMessageKind::REQUEST_INFO:
      data += msg.deviceId.data;
      break;
    case +PeerToBaseMessageKind::DATA:
      data = pack((uint8_t)msg.kind._value) + this->myIdentity->getDeviceId().data;
      data += msg.target.data;
      data += msg.data;
      break;
    case +PeerToBaseMessageKind::USER_AGENT:
      return pack((uint8_t)msg.kind._value) + msg.userAgent;
      break;
    case +PeerToBaseMessageKind::INFO:
      for(InetAddress address : localAddresses) {
        data += address.ip.toBinaryString() + pack((uint16_t)address.port);
      }
      break;
    case +PeerToBaseMessageKind::NAT_INIT:
      data += pack((uint64_t)natInitCounter);
      break;
    case +PeerToBaseMessageKind::NAT_OK_CONFIRM:
      data += pack((uint64_t)natInitCounter);
      break;
    case +PeerToBaseMessageKind::NAT_INIT_TRANSIENT:
      // No extra data needed
      break;
    default:
      LOG_ERROR("tried to serialize unexpected peer to base message type: %s", msg.kind._to_string());
  }

  if(msg.kind != +PeerToBaseMessageKind::DATA)
    data += sign(data, "ng-p2b-msg");

  return data;
}

void NgSocket::udpPacketReceived(InetAddress source, string_view data)
{
  LOG_DEBUG("udp received %s", source.str().c_str());
  if(source == baseUdpAddress) {
    baseMessageReceivedUdp(parseBaseToPeerMessage(data));
  } else {
    if(data[0] == (char)PeerToPeerMessageKind::HELLO || data[0] == (char)PeerToPeerMessageKind::HELLO_REPLY) {
      // "slow" messages are handled on the worker thread to reduce latency
      if(workerQueue.qsize() < WORKER_QUEUE_SIZE) {
        std::string dataCopy = data;
        workerQueue.push([this, source, dataCopy]() { peerMessageReceived(source, parsePeerToPeerMessage(dataCopy)); });
      } else {
        LOG_ERROR("ngsocket worker queue full");
      }
    } else {
      peerMessageReceived(source, parsePeerToPeerMessage(data));
    }
  }
}

void NgSocket::connectToBase()
{
  auto addresses = this->configManager->getBaseAddresses();
  baseAddress = {addresses[baseConnectRetries % addresses.size()], BASESERVER_PORT};

  baseConnectRetries++;

  LOG_INFO("establishing connection to base %s (try %d)", baseAddress.str().c_str(), baseConnectRetries);

  auto dataCallback = [this](string_view data) {
    lastBaseTcpMessage = Port::getCurrentTime();
    this->baseMessageReceivedTcp(parseBaseToPeerMessage(data));
  };

  auto errorCallback = [this](std::shared_ptr<TcpConnection> conn) {
    LOG_CRITICAL("base TCP connection closed");

    if(conn == baseConnection) {
      TcpConnection::close(baseConnection);
      baseConnection = nullptr;  // warning: this will destroy us (the lambda)
      lastBaseTcpAction = Port::getCurrentTime();
    }
  };

  if(baseConnection)
    TcpConnection::close(baseConnection);

  baseConnection = TcpConnection::connect(baseAddress, dataCallback, errorCallback);
  lastBaseTcpAction = Port::getCurrentTime();

  PeerToBaseMessage userAgentMsg = {
      .kind = PeerToBaseMessageKind::USER_AGENT,
      .userAgent = HUSARNET_USER_AGENT,
  };
  sendToBaseTcp(userAgentMsg);
}

void NgSocket::sendToBaseUdp(const PeerToBaseMessage& msg)
{
  if(!baseConnection || cookie.size() == 0)
    return;
  std::string serialized = serializePeerToBaseMessage(msg);
  switch(msg.kind) {
    case +PeerToBaseMessageKind::NAT_INIT:
    case +PeerToBaseMessageKind::NAT_OK_CONFIRM:
    case +PeerToBaseMessageKind::NAT_INIT_TRANSIENT:
      for(InetAddress address : allBaseUdpAddresses) {
        if(msg.kind == +PeerToBaseMessageKind::NAT_INIT_TRANSIENT) {
          address.port = baseTransientPort;
        } else {
          udpSend(address, serialized);
        }
      }
      break;
    default:
      udpSend(baseUdpAddress, serialized);
  }
}

void NgSocket::sendToBaseTcp(const PeerToBaseMessage& msg)
{
  if(!baseConnection || cookie.size() == 0)
    return;
  std::string serialized = serializePeerToBaseMessage(msg);
  TcpConnection::write(baseConnection, serialized);
}

void NgSocket::sendToPeer(InetAddress dest, const PeerToPeerMessage& msg)
{
  std::string serialized = serializePeerToPeerMessage(msg);
  udpSend(dest, std::move(serialized));
}
