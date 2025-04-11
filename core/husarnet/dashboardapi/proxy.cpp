// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "proxy.h"

#include "etl/base64_encoder.h"

namespace dashboardapi {
  etl::string<base64EncodedPublicKeySize> Proxy::encodePublicKey(Identity* identity)
  {
    etl::base64_rfc4648_url_padding_encoder<base64EncodedPublicKeySize> pkEncoder;
    auto pk = identity->getPubkey();
    pkEncoder.encode_final(pk.begin(), pk.end());
    return {pkEncoder.begin(), pkEncoder.size()};
  }

  etl::string<base64EncodedSignatureSize> Proxy::encodeSignature(Identity* identity, const std::string& body)
  {
    etl::base64_rfc4648_url_padding_encoder<base64EncodedSignatureSize> sigEncoder;
    auto sig = identity->sign(body);
    sigEncoder.encode_final(sig.begin(), sig.end());
    return {sigEncoder.begin(), sigEncoder.size()};
  }
}  // namespace dashboardapi