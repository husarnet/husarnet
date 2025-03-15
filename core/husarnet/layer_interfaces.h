// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>

#include "husarnet/ipaddress.h"
#include "husarnet/string_view.h"

// This is a bit confusing unless you know a couple of assumptions, so here
// there are. First of all - this whole internal "stack" is composed of layers.
// Each layer communicates with others through a producer and consumer system.
// Layers are ordered like this:
//
// TunTap (top) (think - this is your "raw data")
// Multicast
// Compression
// Security
// NgSocket (bottom) (think - layer "2", the most packed)
//
// FromUpperConsumer consumes data from ForUpperProducer, thus FromUpperConsumer
// is *lower* than ForUpperProducer FromLowerConsumer consumes data from
// ForLowerProducer, thus ForLowerProducer is *above* LowerConsumer

class FromUpperConsumer {
 public:
  virtual void onUpperLayerData(HusarnetAddress peerAddress, string_view data) = 0;
};

class ForUpperProducer {
 protected:
  std::function<void(HusarnetAddress peerAddress, string_view data)> fromUpperConsumer;

 public:
  ForUpperProducer();

  void setUpperLayerConsumer(std::function<void(HusarnetAddress peerAddress, string_view data)> func);
  void sendToUpperLayer(HusarnetAddress peerAddress, string_view data);
};

class FromLowerConsumer {
 public:
  virtual void onLowerLayerData(HusarnetAddress peerAddress, string_view data) = 0;
};

class ForLowerProducer {
 protected:
  std::function<void(HusarnetAddress peerAddress, string_view data)> fromLowerConsumer;

 public:
  ForLowerProducer();

  void setLowerLayerConsumer(std::function<void(HusarnetAddress peerAddress, string_view data)> func);
  void sendToLowerLayer(HusarnetAddress peerAddress, string_view data);
};

class UpperLayer : public ForLowerProducer, public FromLowerConsumer {};
class LowerLayer : public ForUpperProducer, public FromUpperConsumer {};

class BidirectionalLayer : public UpperLayer, public LowerLayer {};

void stackUpperOnLower(UpperLayer* upper, LowerLayer* lower);
