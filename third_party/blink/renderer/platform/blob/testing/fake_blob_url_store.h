// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BLOB_TESTING_FAKE_BLOB_URL_STORE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BLOB_TESTING_FAKE_BLOB_URL_STORE_H_

#include "third_party/blink/public/mojom/blob/blob_url_store.mojom-blink.h"

#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/blink/renderer/platform/weborigin/kurl_hash.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// Mocked BlobURLStore implementation for testing.
class FakeBlobURLStore : public mojom::blink::BlobURLStore {
 public:
  void Register(mojom::blink::BlobPtr, const KURL&, RegisterCallback) override;
  void Revoke(const KURL&) override;
  void Resolve(const KURL&, ResolveCallback) override;
  void ResolveAsURLLoaderFactory(
      const KURL&,
      network::mojom::blink::URLLoaderFactoryRequest) override;
  void ResolveForNavigation(const KURL&,
                            mojom::blink::BlobURLTokenRequest) override;

  HashMap<KURL, mojom::blink::BlobPtr> registrations;
  Vector<KURL> revocations;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BLOB_TESTING_FAKE_BLOB_URL_STORE_H_
