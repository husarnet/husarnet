// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/peer_transports.h"

// void NgSocket::sendDataToPeer(Peer* peer, string_view data)
// {
//   if(peer->connected) {
//     PeerToPeerMessage msg = {
//         .kind = PeerToPeerMessageKind::DATA,
//         .data = data,
//     };

//     LOG_DEBUG("send to peer %s", peer->targetAddress.str().c_str());
//     sendToPeer(peer->targetAddress, msg);
//   } else {
//     if(!peer->reestablishing ||
//        (Port::getCurrentTime() - peer->lastReestablish > REESTABLISH_TIMEOUT
//        &&
//         peer->failedEstablishments <= MAX_FAILED_ESTABLISHMENTS))
//       attemptReestablish(peer);

//     LOG_DEBUG("send to peer %s tunnelled", std::string(peer->id).c_str());
//     // Not (yet) connected, relay via base.
//     PeerToBaseMessage msg = {
//         .kind = PeerToBaseMessageKind::DATA,
//         .target = peer->id,
//         .data = data,
//     };

//     if(isBaseUdp() &&
//        configStorage.getUserSettingBool(UserSetting::enableUdpTunelling)) {
//       sendToBaseUdp(msg);
//     } else if(configStorage.getUserSettingBool(
//                   UserSetting::enableTcpTunelling)) {
//       sendToBaseTcp(msg);
//     }
//   }

//   if(!peer->isActive())
//     sendInfoRequestToBase(peer->id);

//   peer->lastPacket = Port::getCurrentTime();
// }

// void NgSocket::attemptReestablish(Peer* peer)
// {
//   // TODO long term - if (peer->reestablishing) something;
//   if(!configStorage.getUserSettingBool(UserSetting::enableUdp))
//     return;

//   peer->failedEstablishments++;
//   peer->lastReestablish = Port::getCurrentTime();
//   peer->reestablishing = true;
//   peer->helloCookie = generateRandomString(16);

//   LOG_INFO(
//       "reestablish connection to [%s]",
//       IpAddress::fromBinary(peer->id).str().c_str());

//   std::vector<InetAddress> addresses = peer->targetAddresses;
//   if(peer->linkLocalAddress)
//     addresses.push_back(peer->linkLocalAddress);

//   for(InetAddress address : peer->sourceAddresses)
//     addresses.push_back(address);

//   std::sort(addresses.begin(), addresses.end());
//   addresses.erase(
//       std::unique(addresses.begin(), addresses.end()), addresses.end());

//   std::string msg = "";

//   for(InetAddress address : addresses) {
//     if(address.ip.isFC94())
//       continue;
//     if(!manager->isRealAddressAllowed(address))
//       continue;
//     msg += address.str().c_str();
//     msg += ", ";
//     PeerToPeerMessage response = {
//         .kind = PeerToPeerMessageKind::HELLO,
//         .yourId = peer->id,
//         .helloCookie = peer->helloCookie,
//     };
//     sendToPeer(address, response);

//     if(address == peer->targetAddress)
//       // send the heartbeat twice to the active address
//       sendToPeer(address, response);
//   }
//   LOG_DEBUG("addresses: %s", msg.c_str());
// }

// void NgSocket::peerMessageReceived(
//     InetAddress source,
//     const PeerToPeerMessage& msg)
// {
//   switch(msg.kind) {
//     case +PeerToPeerMessageKind::DATA:
//       peerDataPacketReceived(source, msg.data);
//       break;
//     case +PeerToPeerMessageKind::HELLO:
//       helloReceived(source, msg);
//       break;
//     case +PeerToPeerMessageKind::HELLO_REPLY:
//       helloReplyReceived(source, msg);
//       break;
//     default:
//       LOG_ERROR("unknown message received from peer %s",
//       source.str().c_str());
//   }
// }

// void NgSocket::helloReceived(InetAddress source, const PeerToPeerMessage&
// msg)
// {
//   if(msg.yourId != peerId)
//     return;

//   Peer* peer = peerContainer->getOrCreatePeer(msg.myId);
//   if(peer == nullptr)
//     return;
//   LOG_DEBUG(
//       "HELLO from %s (id: %s, active: %s)", source.str().c_str(),
//       IDSTR(msg.myId), peer->isActive() ? "YES" : "NO");

//   if(!manager->isRealAddressAllowed(source)) {
//     LOG_INFO(
//         "rejected due to real world addresses blacklist %s",
//         source.str().c_str());
//     return;
//   }
//   addSourceAddress(peer, source);

//   if(source.ip.isLinkLocal() && !peer->linkLocalAddress) {
//     peer->linkLocalAddress = source;
//     if(peer->isActive())
//       attemptReestablish(peer);
//   }

//   PeerToPeerMessage reply = {
//       .kind = PeerToPeerMessageKind::HELLO_REPLY,
//       .yourId = msg.myId,
//       .helloCookie = msg.helloCookie,
//   };
//   sendToPeer(source, reply);
// }

// void NgSocket::helloReplyReceived(
//     InetAddress source,
//     const PeerToPeerMessage& msg)
// {
//   if(msg.yourId != peerId)
//     return;

//   LOG_DEBUG(
//       "HELLO-REPLY from %s (%s)", source.str().c_str(),
//       encodeHex(msg.myId).c_str());
//   Peer* peer = peerContainer->getPeer(msg.myId);
//   if(peer == nullptr)
//     return;
//   if(!peer->reestablishing)
//     return;
//   if(peer->helloCookie != msg.helloCookie)
//     return;

//   if(!manager->isRealAddressAllowed(source)) {
//     LOG_INFO(
//         "rejected due to real world addresses blacklist %s",
//         source.str().c_str());
//     return;
//   }

//   int latency = Port::getCurrentTime() - peer->lastReestablish;
//   LOG_DEBUG(
//       " - using this address as target (reply received after %d ms)",
//       latency);
//   peer->targetAddress = source;
//   peer->connected = true;
//   peer->failedEstablishments = 0;
//   peer->reestablishing = false;
// }

// void NgSocket::peerDataPacketReceived(InetAddress source, string_view data)
// {
//   Peer* peer = findPeerBySourceAddress(source);

//   if(peer != nullptr)
//     sendToUpperLayer(peer->id, data);
//   else {
//     LOG_ERROR("unknown UDP data packet (source: %s)", source.str().c_str());
//   }
// }

// void NgSocket::changePeerTargetAddresses(
//     Peer* peer,
//     std::vector<InetAddress> addresses)
// {
//   std::sort(addresses.begin(), addresses.end());
//   if(peer->targetAddresses != addresses) {
//     peer->targetAddresses = addresses;
//     attemptReestablish(peer);
//   }
// }

// PeerToPeerMessage NgSocket::parsePeerToPeerMessage(string_view data)
// {
//   PeerToPeerMessage msg = {
//       .kind = PeerToPeerMessageKind::INVALID,
//   };
//   if(data.size() == 0)
//     return msg;

//   if(data[0] == (char)PeerToPeerMessageKind::HELLO ||
//      data[0] == (char)PeerToPeerMessageKind::HELLO_REPLY) {
//     if(data.size() != 1 + 16 * 3 + 32 + 64)
//       return msg;
//     msg.myId = substr<1, 16>(data);
//     std::string pubkey = data.substr(17, 32);
//     msg.yourId = substr<17 + 32, 16>(data);
//     msg.helloCookie = data.substr(17 + 48, 16);
//     std::string signature = data.substr(17 + 64, 64);

//     if(NgSocketCrypto::pubkeyToPeerId(pubkey) != msg.myId) {
//       LOG_ERROR("invalid pubkey: %s", pubkey.c_str());
//       return msg;
//     }

//     bool ok = GIL::unlocked<bool>([&]() {
//       return NgSocketCrypto::verifySignature(
//           data.substr(0, 17 + 64), "ng-p2p-msg", pubkey, signature);
//     });

//     if(!ok) {
//       LOG_ERROR("invalid signature: %s", signature.c_str());
//       return msg;
//     }
//     msg.kind = PeerToPeerMessageKind::_from_integral(data[0]);
//     return msg;
//   }

//   if(data[0] == (char)PeerToPeerMessageKind::DATA) {
//     msg.kind = PeerToPeerMessageKind::_from_integral(data[0]);
//     msg.data = data.substr(1);
//     return msg;
//   }

//   return msg;
// }

// std::string NgSocket::serializePeerToPeerMessage(const PeerToPeerMessage&
// msg)
// {
//   std::string data;

//   switch(msg.kind) {
//     case +PeerToPeerMessageKind::HELLO:
//     case +PeerToPeerMessageKind::HELLO_REPLY:
//       assert(
//           peerId.size() == 16 && msg.yourId.size() == 16 &&
//           msg.helloCookie.size() == 16 && pubkey.size() == 32);
//       data = pack((uint8_t)msg.kind._value) + peerId + pubkey + msg.yourId +
//              msg.helloCookie;
//       data += GIL::unlocked<std::string>(
//           [&]() { return sign(data, "ng-p2p-msg"); });
//       break;
//     case +PeerToPeerMessageKind::DATA:
//       data = pack((uint8_t)msg.kind._value) + msg.data.str();
//       break;
//     default:
//       abort();
//   }

//   return data;
// }

// void NgSocket::sendToPeer(InetAddress dest, const PeerToPeerMessage& msg)
// {
//   std::string serialized = serializePeerToPeerMessage(msg);
//   udpSend(dest, std::move(serialized));
// }

// void NgSocket::periodicPeer(Peer* peer)
// {
//   if(!peer->isActive()) {
//     peer->connected = false;
//     // TODO long term - send unsubscribe to base
//   } else {
//     if(peer->reestablishing && peer->connected &&
//        Port::getCurrentTime() - peer->lastReestablish > REESTABLISH_TIMEOUT)
//        {
//       peer->connected = false;
//       LOG_WARNING("falling back to relay (peer: %s)", IDSTR(peer->id));
//     }

//     attemptReestablish(peer);
//   }
// }
