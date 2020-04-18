// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_BLOB_STORAGE_BLOB_URL_LOADER_FACTORY_H_
#define CONTENT_BROWSER_BLOB_STORAGE_BLOB_URL_LOADER_FACTORY_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace storage {
class BlobDataHandle;
class BlobStorageContext;
}

namespace content {

// A class for creating URLLoaderFactory for blob scheme.
// There should be one owned per StoragePartition.
//
// This class is deprecated, as it's impossible to use this to load blob URLs
// without running into various race conditions between revoking blob URLs and
// fetching them. Ultimately usage of this class will somehow be replaced with
// usage of storage::BlobURLLoaderFactory.
class BlobURLLoaderFactory
    : public base::RefCountedThreadSafe<BlobURLLoaderFactory,
                                        BrowserThread::DeleteOnIOThread>,
      public network::mojom::URLLoaderFactory {
 public:
  using BlobContextGetter =
      base::OnceCallback<base::WeakPtr<storage::BlobStorageContext>()>;

  static CONTENT_EXPORT scoped_refptr<BlobURLLoaderFactory> Create(
      BlobContextGetter blob_storage_context_getter);

  // Creates a URLLoaderFactory interface pointer for serving blob requests.
  // Called on the UI thread.
  void HandleRequest(network::mojom::URLLoaderFactoryRequest request);

  // Creates a URLLoader for given Blob UUID. This method is supposed to
  // be called on the IO thread.
  // Note that given |request|'s URL is not referenced, but only method and
  // range headers are used.
  static void CreateLoaderAndStart(
      network::mojom::URLLoaderRequest url_loader_request,
      const network::ResourceRequest& request,
      network::mojom::URLLoaderClientPtr client,
      std::unique_ptr<storage::BlobDataHandle> blob_handle);

  // network::mojom::URLLoaderFactory implementation:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest request) override;

 private:
  friend class base::DeleteHelper<BlobURLLoaderFactory>;
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;
  template <typename T, typename... Args>
  friend scoped_refptr<T> base::MakeRefCounted(Args&&... args);

  BlobURLLoaderFactory();
  ~BlobURLLoaderFactory() override;

  void InitializeOnIO(BlobContextGetter blob_storage_context_getter);
  void BindOnIO(network::mojom::URLLoaderFactoryRequest request);

  base::WeakPtr<storage::BlobStorageContext> blob_storage_context_;

  // Used on the IO thread.
  mojo::BindingSet<network::mojom::URLLoaderFactory> loader_factory_bindings_;

  DISALLOW_COPY_AND_ASSIGN(BlobURLLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BLOB_STORAGE_BLOB_URL_LOADER_FACTORY_H_
