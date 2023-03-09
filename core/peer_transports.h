// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include <vector>

#include "husarnet/fstring.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/string_view.h"

#include "enum.h"

class NgSocketManager;

BETTER_ENUM(PeerTransportType, int, TCP = 1, UDP = 2, Tunelled = 3)

BETTER_ENUM(PeerToPeerMessageKind, uint8_t, HELLO, HELLO_REPLY, DATA, INVALID)

struct PeerToPeerMessage {
  PeerToPeerMessageKind kind;

  // hello and hello_reply
  PeerId myId;
  PeerId yourId;
  std::string helloCookie;

  // data message
  string_view data;
};

class PeerTransport : public LowerLayer {
 public:
  virtual void onUpperLayerData(PeerId peerId, string_view data);
};

class PeerTCPTransport : PeerTransport {
 protected:
  NgSocketManager* ngsocket;

 public:
  PeerTCPTransport(NgSocketManager* ngsocket);

  virtual void onUpperLayerData(PeerId peerId, string_view data);
};

class PeerUDPTransport : public PeerTransport {
 protected:
  NgSocketManager* ngsocket;

 public:
  PeerUDPTransport(NgSocketManager* ngsocket);

  virtual void onUpperLayerData(PeerId peerId, string_view data);
};
