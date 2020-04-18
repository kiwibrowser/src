// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/blob/testing/fake_blob_url_store.h"

namespace blink {

void FakeBlobURLStore::Register(mojom::blink::BlobPtr blob,
                                const KURL& url,
                                RegisterCallback callback) {
  registrations.insert(url, std::move(blob));
  std::move(callback).Run();
}

void FakeBlobURLStore::Revoke(const KURL& url) {
  registrations.erase(url);
  revocations.push_back(url);
}

void FakeBlobURLStore::Resolve(const KURL& url, ResolveCallback callback) {
  auto it = registrations.find(url);
  if (it == registrations.end()) {
    std::move(callback).Run(nullptr);
    return;
  }
  mojom::blink::BlobPtr blob;
  it->value->Clone(MakeRequest(&blob));
  std::move(callback).Run(std::move(blob));
}

void FakeBlobURLStore::ResolveAsURLLoaderFactory(
    const KURL&,
    network::mojom::blink::URLLoaderFactoryRequest) {
  NOTREACHED();
}

void FakeBlobURLStore::ResolveForNavigation(const KURL&,
                                            mojom::blink::BlobURLTokenRequest) {
  NOTREACHED();
}

}  // namespace blink
