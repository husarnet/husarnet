// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/compression_layer.h"

#include "husarnet/logging.h"
#include "husarnet/peer.h"
#include "husarnet/peer_container.h"
#include "husarnet/peer_flags.h"
#include "husarnet/util.h"

#ifdef WITH_ZSTD
#include "zstd.h"
#endif

CompressionLayer::CompressionLayer(
    PeerContainer* peerContainer,
    PeerFlags* myFlags)
    : peerContainer(peerContainer), myFlags(myFlags)
{
  this->compressionBuffer.resize(2100);
  this->cleartextBuffer.resize(2010);

#ifndef WITH_ZSTD
  this->myFlags->setFlag(PeerFlag::compression);
#endif
}

bool CompressionLayer::shouldProceed(HusarnetAddress peerId)
{
#ifndef WITH_ZSTD
  return false;
#endif

  auto peer = peerContainer->getPeer(peerId);
  if(!peer->flags.checkFlag(PeerFlag::compression)) {
    return false;
  }

  return true;
}

void CompressionLayer::onUpperLayerData(
    HusarnetAddress peerId,
    string_view data)
{
  if(!shouldProceed(peerId)) {
    sendToLowerLayer(peerId, data);
  }
  // TODO long term - this is left here merely as an example. Reference old code
  // and rewrite this #ifdef WITH_ZSTD
  //   size_t s = ZSTD_compress(
  //       &cleartextBuffer[8], cleartextBuffer.size() - 8, data.data(),
  //       data.size(),
  //       /*level=*/1);
  //   if(ZSTD_isError(s)) {
  //     LOG("ZSTD compression failed");
  //     return;
  //   }

  //   sendToLowerLayer(peerId, cleartextBuffer);
  // #endif
}

void CompressionLayer::onLowerLayerData(
    HusarnetAddress peerId,
    string_view data)
{
  if(!shouldProceed(peerId)) {
    sendToUpperLayer(peerId, data);
  }

  // TODO long term - this is left here merely as an example. Reference old code
  // and rewrite this #ifdef WITH_ZSTD
  //   size_t s = ZSTD_decompress(
  //       data, data.size(), &decryptedBuffer[8],
  //       decryptedSize - 8);
  //   if(ZSTD_isError(s)) {
  //     LOG("ZSTD decompression failed");
  //     return;
  //   }
  //   decryptedData = string_view(compressionBuffer).substr(0, s);
  // #endif
}
