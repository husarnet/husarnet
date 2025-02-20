// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/multicast_layer.h"

#include <array>
#include <string>

#include <stdint.h>

#include "husarnet/fstring.h"
#include "husarnet/identity.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

MulticastLayer::MulticastLayer(
    HusarnetAddress myDeviceId,
    ConfigManager* configmanager)
    : myDeviceId(myDeviceId), configManager(configmanager)
{
}

void MulticastLayer::onLowerLayerData(HusarnetAddress source, string_view data)
{
  std::string packet;
  if(data.size() < 2)
    return;

  uint8_t protocol = data[0];

  if(protocol == 0xFF && data[1] == 0x01) {
    // multicast
    if(data.size() < 20)
      return;

    protocol = data[2];
    string_view mcastAddr = data.substr(3, 16);

    if((uint8_t)mcastAddr[0] != 0xff)
      return;  // important check

    int payloadSize = (int)data.size() - 19;

    packet.reserve(payloadSize + 40);
    packet.resize(8);
    packet[0] = 6 << 4;
    packet[4] = (char)(payloadSize >> 8);
    packet[5] = (char)(payloadSize & 0xFF);
    packet[6] = protocol;
    packet[7] = 3;  // hop limit
    packet +=
        source;  // TODO: ympek: check if binary data is appended correctly
    packet += mcastAddr;
    packet += data.substr(19);

    LOG_INFO("received multicast from %s", source.toString().c_str());

    sendToUpperLayer(IpAddress(), packet);
  } else {
    // unicast
    int payloadSize = (int)data.size() - 1;

    packet.reserve(payloadSize + 40);
    packet.resize(8);
    packet[0] = 6 << 4;
    packet[4] = (char)(payloadSize >> 8);
    packet[5] = (char)(payloadSize & 0xFF);
    packet[6] = protocol;
    packet[7] = 3;  // hop limit
    packet +=
        source;  // TODO : ympek : check if binary data is appended correctly
    packet += this->myDeviceId;
    packet += data.substr(1);

    sendToUpperLayer(source, packet);
  }
}

void MulticastLayer::onUpperLayerData(
    HusarnetAddress target,
    string_view packet)
{
  if(packet.size() <= 40) {
    LOG_WARNING("truncated packet from %s", target.toString().c_str());
    return;
  }
  int version = packet[0] >> 4;
  if(version != 6) {
    LOG_WARNING("bad IP version %d", version);
    return;
  }

  int protocol = packet[6];
  auto srcAddress = fstring<16>(&packet[8]);
  auto dstAddress = fstring<16>(&packet[24]);

  if((uint8_t)dstAddress[0] == 0xff) {
    // multicast (de facto broadcast)
    std::string msgData = "\xff\x01";
    msgData += protocol;
    msgData += dstAddress;
    msgData += packet.substr(40);

    auto dst = this->configManager->getMulticastDestinations(dstAddress);
    for(auto dest : dst) {
      sendToLowerLayer(dest, msgData);
    }

    if(dst.size() > 0) {
      LOG_INFO("send multicast to %d destinations", (int)dst.size());
    }
  }

  if(dstAddress[0] == 0xfc && dstAddress[1] == 0x94) {
    // unicast

    if(srcAddress != this->myDeviceId)
      return;

    string_view msgData = packet.substr(39);
    *(char*)(&msgData[0]) =
        (char)protocol;  // a bit hacky, but we assume we can modify `packet`
    sendToLowerLayer(dstAddress, msgData);
  }
}
