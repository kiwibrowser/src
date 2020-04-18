// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(jww) The original Blink-style header guard for this file conflicts with
// the header guard in Source/modules/crypto/Crypto.h, so this is a
// Chromium-style header guard instead. There is now a bug
// (https://crbug.com/360121) to track a proposal to change all header guards
// to a similar style. Thus, whenever that is resolved, this header guard
// should be changed to whatever style is agreed upon.
#ifndef SOURCE_PLATFORM_CRYPTO_H_
#define SOURCE_PLATFORM_CRYPTO_H_

#include <memory>
#include "third_party/blink/public/platform/web_crypto.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/string_hasher.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

static const size_t kMaxDigestSize = 64;
typedef Vector<uint8_t, kMaxDigestSize> DigestValue;

const size_t kSha1HashSize = 20;
enum HashAlgorithm {
  kHashAlgorithmSha1,
  kHashAlgorithmSha256,
  kHashAlgorithmSha384,
  kHashAlgorithmSha512
};

PLATFORM_EXPORT bool ComputeDigest(HashAlgorithm,
                                   const char* digestable,
                                   size_t length,
                                   DigestValue& digest_result);
// Note: this will never return null.
PLATFORM_EXPORT std::unique_ptr<WebCryptoDigestor> CreateDigestor(
    HashAlgorithm);
PLATFORM_EXPORT void FinishDigestor(WebCryptoDigestor*,
                                    DigestValue& digest_result);

}  // namespace blink

namespace WTF {

struct DigestValueHash {
  STATIC_ONLY(DigestValueHash);
  static unsigned GetHash(const blink::DigestValue& v) {
    return StringHasher::ComputeHash(v.data(), v.size());
  }
  static bool Equal(const blink::DigestValue& a, const blink::DigestValue& b) {
    return a == b;
  };
  static const bool safe_to_compare_to_empty_or_deleted = true;
};
template <>
struct DefaultHash<blink::DigestValue> {
  STATIC_ONLY(DefaultHash);
  typedef DigestValueHash Hash;
};

template <>
struct DefaultHash<blink::HashAlgorithm> {
  STATIC_ONLY(DefaultHash);
  typedef IntHash<blink::HashAlgorithm> Hash;
};
template <>
struct HashTraits<blink::HashAlgorithm>
    : UnsignedWithZeroKeyHashTraits<blink::HashAlgorithm> {
  STATIC_ONLY(HashTraits);
};

}  // namespace WTF
#endif  // SOURCE_PLATFORM_CRYPTO_H_
