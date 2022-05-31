// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include "husarnet/device_id.h"
#include "husarnet/string_view.h"
#include "husarnet/util.h"

// This is a bit confusing unless you know a couple of assumptions, so here
// there are. First of all - this whole internal "stack" is composed of layers.
// Each layer communicates with others through a producer and consumer system.
// Layers are ordered like this:
// TunTap (top) (think - this is your "raw data")
// Multicast
// Compression
// Security
// NgSocket (bottom) (think - layer "2", the most packed)
// FromUpperConsumer consumes data from ForUpperProducer, thus FromUpperConsumer
// is *lower* than ForUpperProducer FromLowerConsumer consumes data from
// ForLowerProducer, thus ForLowerProducer is *above* LoverConsumer

// TODO rename this file to something like
// layer_glue/layer_definitions/layer_interface/…

class FromUpperConsumer {
 public:
  virtual void onUpperLayerData(DeviceId source, string_view data) = 0;
};

class ForUpperProducer {
 protected:
  std::function<void(DeviceId source, string_view data)> fromUpperConsumer =
      [](DeviceId source, string_view data) {
        LOG("dropping frame for upper layer");
      };

 public:
  void setUpperLayerConsumer(
      std::function<void(DeviceId, string_view data)> func)
  {
    fromUpperConsumer = func;
  }

  void sendToUpperLayer(DeviceId source, string_view data)
  {
    fromUpperConsumer(source, data);
  }
};

class FromLowerConsumer {
 public:
  virtual void onLowerLayerData(DeviceId source, string_view data) = 0;
};

class ForLowerProducer {
 protected:
  std::function<void(DeviceId source, string_view data)> fromLowerConsumer =
      [](DeviceId source, string_view data) {
        LOG("dropping frame for lower layer");
      };

 public:
  void setLowerLayerConsumer(
      std::function<void(DeviceId target, string_view data)> func)
  {
    fromLowerConsumer = func;
  }

  void sendToLowerLayer(DeviceId target, string_view data)
  {
    fromLowerConsumer(target, data);
  }
};

class HigherLayer : public ForLowerProducer, public FromLowerConsumer {
};
class LowerLayer : public ForUpperProducer, public FromUpperConsumer {
};

class BidirectionalLayer : public HigherLayer, public LowerLayer {
};

inline void stackHigherOnLower(HigherLayer* higher, LowerLayer* lower)
{
  higher->setLowerLayerConsumer(std::bind(
      &FromUpperConsumer::onUpperLayerData, lower, std::placeholders::_1,
      std::placeholders::_2));

  lower->setUpperLayerConsumer(std::bind(
      &FromLowerConsumer::onLowerLayerData, higher, std::placeholders::_1,
      std::placeholders::_2));
}
