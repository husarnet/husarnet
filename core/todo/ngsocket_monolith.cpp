// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <algorithm>
#include <array>
#include <mutex>
#include <unordered_map>

#include <assert.h>
#include <stdlib.h>

#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/privileged_interface.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/config_storage.h"
#include "husarnet/fstring.h"
#include "husarnet/gil.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/ngsocket.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/ngsocket_messages.h"
#include "husarnet/peer.h"
#include "husarnet/peer_container.h"
#include "husarnet/queue.h"
#include "husarnet/util.h"

namespace OsSocket {
  struct FramedTcpConnection;
}  // namespace OsSocket

#if defined(ESP_PLATFORM)
const int WORKER_QUEUE_SIZE = 16;
#else
const int WORKER_QUEUE_SIZE = 256;
#endif

using mutex_guard = std::lock_guard<std::recursive_mutex>;

using namespace OsSocket;

// TODO get rid of this PeerId.getAddress().toString() should already implement
// this
std::string idstr(PeerId id)
{
  return IpAddress::fromBinary(id).str();
}

// TODO I mean… can we get rid of this too please?
#define IDSTR(id) idstr(id).c_str()

NgSocket::NgSocket(HusarnetManager* manager)
    : manager(manager), configStorage(manager->getConfigStorage())
{
  identity = manager->getIdentity();
  peerContainer = manager->getPeerContainer();

  pubkey = manager->getIdentity()->getPubkey();
  peerId = manager->getIdentity()->getPeerId();
  init();
}

void NgSocket::periodic()
{
  if(Port::getCurrentTime() < lastPeriodic + 1000)
    return;
  lastPeriodic = Port::getCurrentTime();

  if(reloadLocalAddresses()) {
    // new addresses, accelerate reconnection
    auto addresses_string = std::transform_reduce(
        localAddresses.begin(), localAddresses.end(), std::string(""),
        [](const std::string& a, const std::string& b) {
          return a + " | " + b;
        },
        [](InetAddress addr) { return addr.str(); });
    LOG_INFO(
        "Local IP address change detected, new addresses: %s",
        addresses_string.c_str());
    requestRefresh();
    if(Port::getCurrentTime() - lastBaseTcpAction > NAT_INIT_TIMEOUT)
      connectToBase();
  }

  if(Port::getCurrentTime() - lastRefresh > REFRESH_TIMEOUT)
    requestRefresh();

  if(!natInitConfirmed &&
     Port::getCurrentTime() - lastNatInitSent > NAT_INIT_TIMEOUT) {
    // retry nat init in case the packet gets lost
    sendNatInitToBase();
  }

  if(Port::getCurrentTime() - lastBaseTcpAction >
     (baseConnection ? TCP_PONG_TIMEOUT : NAT_INIT_TIMEOUT))
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

  return BaseConnectionType::NONE;
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
    GIL::lock();
    f();
    GIL::unlock();
  }
}

void NgSocket::refresh()
{
  // release the lock around every operation to reduce latency
  lastRefresh = Port::getCurrentTime();
  sendLocalAddressesToBase();

  GIL::yield();

  sendNatInitToBase();
  sendMulticast();

  for(Peer* peer : iteratePeers()) {
    GIL::yield();
    periodicPeer(peer);
  }
}

void NgSocket::onUpperLayerData(PeerId peerId, string_view data)
{
  Peer* peer = peerContainer->getOrCreatePeer(peerId);
  if(peer != nullptr)
    sendDataToPeer(peer, data);
}

bool NgSocket::isBaseUdp()
{
  return Port::getCurrentTime() - lastNatInitConfirmation < UDP_BASE_TIMEOUT;
}

void NgSocket::baseMessageReceivedUdp(const BaseToPeerMessage& msg)
{
  switch(msg.kind) {
    case +BaseToPeerMessageKind::DATA:
      sendToUpperLayer(msg.source, msg.data);
      break;
    case +BaseToPeerMessageKind::NAT_OK: {
      if(lastNatInitConfirmation == 0) {
        LOG_INFO(
            "UDP connection to base server established, server address: %s",
            baseAddress.str().c_str());
      }
      lastNatInitConfirmation = Port::getCurrentTime();
      natInitConfirmed = true;

      PeerToBaseMessage resp{
          .kind = PeerToBaseMessageKind::NAT_OK_CONFIRM,
      };
      sendToBaseUdp(resp);
    } break;
    default:
      LOG_ERROR(
          "received invalid UDP message from base (kind: %s)",
          msg.kind._to_string());
  }
}

void NgSocket::baseMessageReceivedTcp(const BaseToPeerMessage& msg)
{
  lastBaseTcpAction = lastBaseTcpMessage = Port::getCurrentTime();
  baseConnectRetries = 0;

  switch(msg.kind) {
    case +BaseToPeerMessageKind::DATA:
      sendToUpperLayer(msg.source, msg.data);
      break;
    case +BaseToPeerMessageKind::HELLO:
      LOG_INFO(
          "TCP connection to base server established, server address: %s",
          baseAddress.str().c_str());
      LOG_DEBUG("received hello cookie %s", encodeHex(msg.cookie).c_str());
      cookie = msg.cookie;
      resendInfoRequests();
      sendLocalAddressesToBase();
      requestRefresh();
      break;
    case +BaseToPeerMessageKind::DEVICE_ADDRESSES:
      if(configStorage.getUserSettingBool(UserSetting::enableUdp)) {
        Peer* peer = peerContainer->getPeer(msg.peerId);
        if(peer != nullptr)
          changePeerTargetAddresses(peer, msg.addresses);
      }
      break;
    case +BaseToPeerMessageKind::STATE:
      if(configStorage.getUserSettingBool(UserSetting::enableUdp)) {
        allBaseUdpAddresses = msg.udpAddress;
        baseUdpAddress = msg.udpAddress[0];
        LOG_DEBUG(
            "received base UDP address: %s", baseUdpAddress.str().c_str());

        if(msg.natTransientRange.first != 0 &&
           msg.natTransientRange.second >= msg.natTransientRange.first) {
          LOG_DEBUG(
              "received base transient range: %d %d",
              msg.natTransientRange.first, msg.natTransientRange.second);
          baseTransientRange = msg.natTransientRange;
          if(baseTransientPort == 0) {
            baseTransientPort = baseTransientRange.first;
          }
        }
      }
      break;
    case +BaseToPeerMessageKind::REDIRECT:
      baseAddress = msg.newBaseAddress;
      LOG_INFO("redirected to new base server: %s", baseAddress.str().c_str());
      connectToBase();
      break;
    default:
      LOG_ERROR(
          "received invalid TCP message from base (kind: %s)",
          msg.kind._to_string());
  }
}

void NgSocket::sendNatInitToBase()
{
  if(!configStorage.getUserSettingBool(UserSetting::enableUdp))
    return;
  if(cookie.size() == 0)
    return;

  lastNatInitSent = Port::getCurrentTime();
  natInitConfirmed = false;

  PeerToBaseMessage msg = {
      .kind = PeerToBaseMessageKind::NAT_INIT,
      .peerId = peerId,
      .counter = natInitCounter,
  };
  natInitCounter++;

  sendToBaseUdp(msg);

  if(baseTransientPort != 0) {
    PeerToBaseMessage msg2 = {
        .kind = PeerToBaseMessageKind::NAT_INIT_TRANSIENT,
        .peerId = peerId,
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
  };

  if(configStorage.getUserSettingBool(UserSetting::enableUdp))
    msg.addresses = localAddresses;
  sendToBaseTcp(msg);
}

void NgSocket::sendInfoRequestToBase(PeerId id)
{
  LOG_DEBUG("info request %s", IDSTR(id));
  PeerToBaseMessage msg = {
      .kind = PeerToBaseMessageKind::REQUEST_INFO,
      .peerId = id,
  };
  sendToBaseTcp(msg);
}

// crypto
std::string NgSocket::sign(const std::string& data, const std::string& kind)
{
  return NgSocketCrypto::sign(data, kind, identity);
}

// data structure manipulation

Peer* cachedPeer = nullptr;
PeerId cachedPeerId;

bool NgSocket::reloadLocalAddresses()
{
  std::vector<InetAddress> newAddresses;

  for(IpAddress address : Privileged::getLocalAddresses()) {
    if(address.isFC94()) {
      continue;
    }

    newAddresses.push_back(InetAddress{address, (uint16_t)sourcePort});
  }

  if(configStorage.getUserSettingInet(UserSetting::extraAdvertisedAddress)) {
    newAddresses.push_back(
        configStorage.getUserSettingInet(UserSetting::extraAdvertisedAddress));
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
    msg.peerId = std::string(substr<1, 16>(data));
    for(int i = 17; i + 18 <= data.size(); i += 18) {
      msg.addresses.push_back(InetAddress{
          IpAddress::fromBinary(data.substr(i, 16)),
          unpack<uint16_t>(data.substr(i + 16, 2))});
    }
  } else if(data[0] == (char)BaseToPeerMessageKind::DATA) {
    if(data.size() <= 17)
      return msg;
    msg.source = std::string(substr<1, 16>(data));
    msg.data = data.substr(17);
  } else if(data[0] == (char)BaseToPeerMessageKind::NAT_OK) {
  } else if(data[0] == (char)BaseToPeerMessageKind::STATE) {
    int i = 1;

    while(i + 18 <= data.size() && msg.udpAddress.size() < 5) {
      msg.udpAddress.push_back(InetAddress{
          IpAddress::fromBinary(data.substr(i, 16)),
          unpack<uint16_t>(data.substr(i + 16, 2))});
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

  msg.kind = BaseToPeerMessageKind::_from_integral(data[0]);
  return msg;
}

std::string NgSocket::serializePeerToBaseMessage(const PeerToBaseMessage& msg)
{
  assert(peerId.size() == 16 && cookie.size() == 16 && pubkey.size() == 32);
  std::string data = pack((uint8_t)msg.kind._value) + this->peerId +
                     this->pubkey + this->cookie;

  switch(msg.kind) {
    case +PeerToBaseMessageKind::REQUEST_INFO:
      data += fstring<16>(msg.peerId);
      break;
    case +PeerToBaseMessageKind::DATA:
      data = pack((uint8_t)msg.kind._value) + this->peerId;
      data += fstring<16>(msg.target);
      data += msg.data;
      break;
    case +PeerToBaseMessageKind::USER_AGENT:
      return pack((uint8_t)msg.kind._value) + msg.userAgent;
      break;
    case +PeerToBaseMessageKind::INFO:
      for(InetAddress address : localAddresses) {
        data += address.ip.toBinary() + pack((uint16_t)address.port);
      }
      break;
    case +PeerToBaseMessageKind::NAT_INIT:
      data += pack((uint64_t)natInitCounter);
      break;
    case +PeerToBaseMessageKind::NAT_OK_CONFIRM:
      data += pack((uint64_t)natInitCounter);
      break;
    default:
      LOG_ERROR(
          "tried to serialize unexpected peer to base message type: %s",
          msg.kind._to_string());
  }

  if(msg.kind != +PeerToBaseMessageKind::DATA)
    data += sign(data, "ng-p2b-msg");

  return data;
}

void NgSocket::connectToBase()
{
  if(configStorage.getUserSettingInet(UserSetting::overrideBaseAddress)) {
    baseAddress =
        configStorage.getUserSettingInet(UserSetting::overrideBaseAddress);
  } else {
    if(baseConnectRetries > 2) {
      // failover
      auto baseTcpAddressesTmp = manager->getBaseServerAddresses();
      baseAddress = {
          baseTcpAddressesTmp[baseConnectRetries % baseTcpAddressesTmp.size()],
          BASESERVER_PORT};
      LOG_WARNING(
          "retrying with fallback base address: %s", baseAddress.str().c_str());
    }
  }
  baseConnectRetries++;
  if(!baseAddress) {
    LOG_DEBUG("waiting until base address is resolved...");
    return;
  }

  LOG_INFO("establishing connection to base %s", baseAddress.str().c_str());

  auto dataCallback = [this](string_view data) {
    lastBaseTcpMessage = Port::getCurrentTime();
    this->baseMessageReceivedTcp(parseBaseToPeerMessage(data));
  };

  auto errorCallback = [this](std::shared_ptr<FramedTcpConnection> conn) {
    LOG_CRITICAL("base TCP connection closed");

    if(conn == baseConnection) {
      close(baseConnection);
      baseConnection = nullptr;  // warning: this will destroy us (the lambda)
      lastBaseTcpAction = Port::getCurrentTime();
    }
  };

  if(baseConnection)
    close(baseConnection);

  baseConnection = tcpConnect(baseAddress, dataCallback, errorCallback);
  lastBaseTcpAction = Port::getCurrentTime();

  PeerToBaseMessage userAgentMsg = {
      .kind = PeerToBaseMessageKind::USER_AGENT,
      .userAgent = manager->getUserAgent(),
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
  write(baseConnection, serialized, msg.kind != +PeerToBaseMessageKind::DATA);
}
