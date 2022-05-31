// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/compression_layer.h"
#ifdef WITH_ZSTD
#include "zstd.h"
#endif

// TODO this whole file

// string_view decryptedData;
// if(options->compressionEnabled && (peer->flags & FLAG_COMPRESSION)) {
// #ifdef WITH_ZSTD
//   size_t s = ZSTD_decompress(
//       &compressBuffer[0], compressBuffer.size(), &decryptedBuffer[8],
//       decryptedSize - 8);
//   if(ZSTD_isError(s)) {
//     LOG("ZSTD decompression failed");
//     return;
//   }
//   decryptedData = string_view(compressBuffer).substr(0, s);
// #else
//   abort();
// #endif
// } else {
//   decryptedData = string_view(decryptedBuffer).substr(8, decryptedSize - 8);
// }

// if(options->compressionEnabled && (peer->flags & FLAG_COMPRESSION)) {
// #ifdef WITH_ZSTD
//   size_t s = ZSTD_compress(
//       &cleartextBuffer[8], cleartextBuffer.size() - 8, data.data(),
//       data.size(),
//       /*level=*/1);
//   if(ZSTD_isError(s)) {
//     LOG("ZSTD compression failed");
//     return;
//   }
//   cleartextSize = (int)s + 8;
// #else
//   abort();
// #endif
// } else {
//   memcpy(&cleartextBuffer[8], data.data(), data.size());
// }
