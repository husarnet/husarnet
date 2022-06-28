// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/l2unwrapper.h"

#include "husarnet/ports/port.h"

#include "husarnet/util.h"

static const std::string peerMacAddr = decodeHex("525400fc944d");
static const std::string defaultSelfMacAddr = decodeHex("525400fc944c");

struct UnwrapperImpl : NgSocket, NgSocketDelegate {
  NgSocket* socket;
  std::string selfMacAddr;

  void periodic() override
  {
    socket->periodic();
  }

  void onDataPacket(DeviceId source, string_view data) override
  {
    // L3 packet received, wrap in in L2 frame
    std::string wrapped;
    wrapped += selfMacAddr;
    wrapped += peerMacAddr;
    wrapped += string_view("\x86\xdd", 2);
    wrapped += data;
    delegate->onDataPacket(source, wrapped);
  }

  void sendDataPacket(DeviceId target, string_view packet) override
  {
    // L2 packet received, unwrap L2 frame and transmit L3 futher
    // LOG("to: %s (exp: %s), from: %s (exp: %s), proto: %s",
    // encodeHex(packet.substr(0, 6)).c_str(), encodeHex(peerMacAddr).c_str(),
    // encodeHex(packet.substr(6, 6)).c_str(), encodeHex(selfMacAddr).c_str(),
    //    encodeHex(packet.substr(12, 2)).c_str());

    if(packet.substr(0, 6) == peerMacAddr &&
       packet.substr(6, 6) == selfMacAddr &&
       packet.substr(12, 2) == string_view("\x86\xdd", 2)) {
      socket->sendDataPacket(target, packet.substr(14));
    }
  }
};

namespace L2Unwrapper {
  NgSocket* wrap(NgSocket* sock, std::string mac)
  {
    auto dev = new UnwrapperImpl;
    dev->selfMacAddr = (mac.size() > 0) ? mac : defaultSelfMacAddr;
    dev->socket = sock;
    sock->delegate = dev;
    return dev;
  }
}  // namespace L2Unwrapper
