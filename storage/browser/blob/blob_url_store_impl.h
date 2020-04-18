// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_BLOB_URL_STORE_IMPL_H_
#define STORAGE_BROWSER_BLOB_BLOB_URL_STORE_IMPL_H_

#include <memory>
#include "storage/browser/blob/blob_registry_impl.h"
#include "storage/browser/storage_browser_export.h"
#include "third_party/blink/public/mojom/blob/blob_url_store.mojom.h"

namespace storage {

class BlobStorageContext;

class STORAGE_EXPORT BlobURLStoreImpl : public blink::mojom::BlobURLStore {
 public:
  BlobURLStoreImpl(base::WeakPtr<BlobStorageContext> context,
                   BlobRegistryImpl::Delegate* delegate);
  ~BlobURLStoreImpl() override;

  void Register(blink::mojom::BlobPtr blob,
                const GURL& url,
                RegisterCallback callback) override;
  void Revoke(const GURL& url) override;
  void Resolve(const GURL& url, ResolveCallback callback) override;
  void ResolveAsURLLoaderFactory(
      const GURL& url,
      network::mojom::URLLoaderFactoryRequest request) override;
  void ResolveForNavigation(const GURL& url,
                            blink::mojom::BlobURLTokenRequest token) override;

 private:
  void RegisterWithUUID(blink::mojom::BlobPtr blob,
                        const GURL& url,
                        RegisterCallback callback,
                        const std::string& uuid);

  base::WeakPtr<BlobStorageContext> context_;
  BlobRegistryImpl::Delegate* delegate_;

  std::set<GURL> urls_;

  base::WeakPtrFactory<BlobURLStoreImpl> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(BlobURLStoreImpl);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_BLOB_BLOB_URL_STORE_IMPL_H_
