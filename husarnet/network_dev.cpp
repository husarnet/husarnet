// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/network_dev.h"
#include "husarnet/ports/port.h"
#include "husarnet/util.h"

struct NetworkDevImpl : NgSocket, NgSocketDelegate {
  NgSocket* socket;
  DeviceId deviceId;
  std::function<std::vector<DeviceId>(DeviceId)> getMulticastDestinations;

  void periodic() override { socket->periodic(); }

  void onDataPacket(DeviceId source, string_view data) override
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
      packet += source;
      packet += mcastAddr;
      packet += data.substr(19);

      LOG("received multicast from %s", encodeHex(source).c_str());

      delegate->onDataPacket(BadDeviceId, packet);
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
      packet += source;
      packet += deviceId;
      packet += data.substr(1);

      delegate->onDataPacket(BadDeviceId, packet);
    }
  }

  void sendDataPacket(DeviceId target, string_view packet) override
  {
    if(packet.size() <= 40) {
      LOG("truncated packet");
      return;
    }
    int version = packet[0] >> 4;
    if(version != 6) {
      LOG("bad IP version %d", version);
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

      auto dst = this->getMulticastDestinations(dstAddress);
      for(DeviceId dest : dst) {
        socket->sendDataPacket(dest, msgData);
      }

      LOG("send multicast to %d destinations", (int)dst.size());
    }

    if(dstAddress[0] == 0xfc && dstAddress[1] == 0x94) {
      // unicast

      if(srcAddress != deviceId)
        return;

      string_view msgData = packet.substr(39);
      *(char*)(&msgData[0]) =
          (char)protocol;  // a bit hacky, but we assume we can modify `packet`
      socket->sendDataPacket(dstAddress, msgData);
    }
  }
};

NgSocket* NetworkDev::wrap(
    DeviceId deviceId,
    NgSocket* sock,
    std::function<std::vector<DeviceId>(DeviceId)> getMulticastDestinations)
{
  auto dev = new NetworkDevImpl;
  dev->deviceId = deviceId;
  dev->socket = sock;
  dev->getMulticastDestinations = getMulticastDestinations;
  sock->delegate = dev;
  return dev;
}
