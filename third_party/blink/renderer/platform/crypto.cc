// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/crypto.h"

#include <memory>
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_crypto.h"
#include "third_party/blink/public/platform/web_crypto_algorithm.h"

namespace blink {

static WebCryptoAlgorithmId ToWebCryptoAlgorithmId(HashAlgorithm algorithm) {
  switch (algorithm) {
    case kHashAlgorithmSha1:
      return kWebCryptoAlgorithmIdSha1;
    case kHashAlgorithmSha256:
      return kWebCryptoAlgorithmIdSha256;
    case kHashAlgorithmSha384:
      return kWebCryptoAlgorithmIdSha384;
    case kHashAlgorithmSha512:
      return kWebCryptoAlgorithmIdSha512;
  };

  NOTREACHED();
  return kWebCryptoAlgorithmIdSha256;
}

bool ComputeDigest(HashAlgorithm algorithm,
                   const char* digestable,
                   size_t length,
                   DigestValue& digest_result) {
  WebCryptoAlgorithmId algorithm_id = ToWebCryptoAlgorithmId(algorithm);
  WebCrypto* crypto = Platform::Current()->Crypto();
  unsigned char* result;
  unsigned result_size;

  DCHECK(crypto);

  std::unique_ptr<WebCryptoDigestor> digestor =
      crypto->CreateDigestor(algorithm_id);
  DCHECK(digestor);
  if (!digestor->Consume(reinterpret_cast<const unsigned char*>(digestable),
                         length) ||
      !digestor->Finish(result, result_size))
    return false;

  digest_result.Append(static_cast<uint8_t*>(result), result_size);
  return true;
}

std::unique_ptr<WebCryptoDigestor> CreateDigestor(HashAlgorithm algorithm) {
  return Platform::Current()->Crypto()->CreateDigestor(
      ToWebCryptoAlgorithmId(algorithm));
}

void FinishDigestor(WebCryptoDigestor* digestor, DigestValue& digest_result) {
  unsigned char* result = nullptr;
  unsigned result_size = 0;

  if (!digestor->Finish(result, result_size))
    return;

  DCHECK(result);

  digest_result.Append(static_cast<uint8_t*>(result), result_size);
}

}  // namespace blink
