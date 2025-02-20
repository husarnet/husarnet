// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/layer_interfaces.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

ForUpperProducer::ForUpperProducer()
    : fromUpperConsumer([](HusarnetAddress peerId, string_view data) {
        LOG_DEBUG(
            "dropping frame for upper layer, peer: %s",
            peerId.toString().c_str());
      })
{
}

void ForUpperProducer::setUpperLayerConsumer(
    std::function<void(HusarnetAddress peerId, string_view data)> func)
{
  fromUpperConsumer = func;
}

void ForUpperProducer::sendToUpperLayer(
    HusarnetAddress peerId,
    string_view data)
{
  fromUpperConsumer(peerId, data);
}

ForLowerProducer::ForLowerProducer()
    : fromLowerConsumer([](HusarnetAddress peerId, string_view data) {
        LOG_DEBUG(
            "dropping frame for lower layer, peer: %s",
            peerId.toString().c_str());
      }){};

void ForLowerProducer::setLowerLayerConsumer(
    std::function<void(HusarnetAddress peerId, string_view data)> func)
{
  fromLowerConsumer = func;
}

void ForLowerProducer::sendToLowerLayer(
    HusarnetAddress peerId,
    string_view data)
{
  fromLowerConsumer(peerId, data);
}

void stackUpperOnLower(UpperLayer* upper, LowerLayer* lower)
{
  upper->setLowerLayerConsumer(std::bind(
      &FromUpperConsumer::onUpperLayerData, lower, std::placeholders::_1,
      std::placeholders::_2));

  lower->setUpperLayerConsumer(std::bind(
      &FromLowerConsumer::onLowerLayerData, upper, std::placeholders::_1,
      std::placeholders::_2));
}
