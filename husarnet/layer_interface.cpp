// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/layer_interfaces.h"
#include "husarnet/util.h"

ForUpperProducer::ForUpperProducer()
    : fromUpperConsumer([](DeviceId peerId, string_view data) {
        LOG("dropping frame for upper layer");
      })
{
}

void ForUpperProducer::setUpperLayerConsumer(
    std::function<void(DeviceId peerId, string_view data)> func)
{
  fromUpperConsumer = func;
}

void ForUpperProducer::sendToUpperLayer(DeviceId peerId, string_view data)
{
  fromUpperConsumer(peerId, data);
}

ForLowerProducer::ForLowerProducer()
    : fromLowerConsumer([](DeviceId peerId, string_view data) {
        LOG("dropping frame for lower layer");
      }){};

void ForLowerProducer::setLowerLayerConsumer(
    std::function<void(DeviceId peerId, string_view data)> func)
{
  fromLowerConsumer = func;
}

void ForLowerProducer::sendToLowerLayer(DeviceId peerId, string_view data)
{
  fromLowerConsumer(peerId, data);
}

void stackHigherOnLower(HigherLayer* higher, LowerLayer* lower)
{
  // TODO ympek
  // crash on Windows
  higher->setLowerLayerConsumer(std::bind(
      &FromUpperConsumer::onUpperLayerData, lower, std::placeholders::_1,
      std::placeholders::_2));

  lower->setUpperLayerConsumer(std::bind(
      &FromLowerConsumer::onLowerLayerData, higher, std::placeholders::_1,
      std::placeholders::_2));
}
