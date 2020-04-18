// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_URL_LOADER_FACTORY_GETTER_H_
#define CONTENT_BROWSER_URL_LOADER_FACTORY_GETTER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace network {
class SharedURLLoaderFactoryInfo;
class SharedURLLoaderFactory;
}  // namespace network

namespace content {

class StoragePartitionImpl;

// Holds on to URLLoaderFactory for a given StoragePartition and allows code
// running on the IO thread to access them. Note these are the factories used by
// the browser process for frame requests.
class URLLoaderFactoryGetter
    : public base::RefCountedThreadSafe<URLLoaderFactoryGetter,
                                        BrowserThread::DeleteOnIOThread> {
 public:
  CONTENT_EXPORT URLLoaderFactoryGetter();

  // Initializes this object on the UI thread. The |partition| is used to
  // initialize the URLLoaderFactories for the network service, AppCache, and
  // ServiceWorkers, and will be cached to recover from connection error.
  // After Initialize(), you can get URLLoaderFactories from this
  // getter. However, any messages on it will be queued until
  // HandleFactoryRequests() is called.
  void Initialize(StoragePartitionImpl* partition);

  // Called on the UI thread to actually instantiate factories whose pointers
  // are to be exposed by this getter.  This must be called after NetworkContext
  // is initialized.
  // TODO(shimazu): Remove this once NetworkService is shipped.
  void HandleFactoryRequests();

  // Clear the cached pointer to |StoragePartitionImpl| on the UI thread. Should
  // be called when the partition is going away.
  void OnStoragePartitionDestroyed();

  // Called on the IO thread to get a shared wrapper to this
  // URLLoaderFactoryGetter, which can be used to access the URLLoaderFactory
  // to the network service and supports auto-reconnect after crash.
  CONTENT_EXPORT scoped_refptr<network::SharedURLLoaderFactory>
  GetNetworkFactory();

  // Called on the UI thread to get an info that holds a reference to this
  // URLLoaderFactoryGetter, which can be used to construct a similar
  // SharedURLLoaderFactory as returned from |GetNetworkFactory()| on IO thread.
  CONTENT_EXPORT std::unique_ptr<network::SharedURLLoaderFactoryInfo>
  GetNetworkFactoryInfo();

  // Called on the IO thread. Will clone the internal factory to the network
  // service which doesn't support auto-reconnect after crash. Useful for
  // one-off requests (e.g. A single navigation) to avoid additional mojo hop.
  CONTENT_EXPORT void CloneNetworkFactory(
      network::mojom::URLLoaderFactoryRequest network_factory_request);

  // Called on the IO thread to get the URLLoaderFactory to the blob service.
  // Must be used only if the network service or servicified service worker is
  // enabled. The pointer shouldn't be cached.
  CONTENT_EXPORT network::mojom::URLLoaderFactory* GetBlobFactory();

  // Overrides the network URLLoaderFactory for subsequent requests. Passing a
  // null pointer will restore the default behavior.
  CONTENT_EXPORT void SetNetworkFactoryForTesting(
      network::mojom::URLLoaderFactory* test_factory);

  CONTENT_EXPORT network::mojom::URLLoaderFactoryPtr*
  original_network_factory_for_testing() {
    return &network_factory_;
  }

  // When this global function is set, if GetURLLoaderFactory is called and
  // |test_factory_| is null, then the callback will be run. This method must be
  // called either on the IO thread or before threads start. This callback is
  // run on the IO thread.
  using GetNetworkFactoryCallback = base::RepeatingCallback<void(
      URLLoaderFactoryGetter* url_loader_factory_getter)>;
  CONTENT_EXPORT static void SetGetNetworkFactoryCallbackForTesting(
      const GetNetworkFactoryCallback& get_network_factory_callback);

  // Call |network_factory_.FlushForTesting()| on IO thread. For test use only.
  CONTENT_EXPORT void FlushNetworkInterfaceOnIOThreadForTesting();

 private:
  class URLLoaderFactoryForIOThreadInfo;
  class URLLoaderFactoryForIOThread;

  friend class base::DeleteHelper<URLLoaderFactoryGetter>;
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;

  CONTENT_EXPORT ~URLLoaderFactoryGetter();
  void InitializeOnIOThread(
      network::mojom::URLLoaderFactoryPtrInfo network_factory,
      network::mojom::URLLoaderFactoryPtrInfo blob_factory);

  // Send |network_factory_request| to cached |StoragePartitionImpl|.
  void HandleNetworkFactoryRequestOnUIThread(
      network::mojom::URLLoaderFactoryRequest network_factory_request);

  // Called on the IO thread to get the URLLoaderFactory to the network service.
  // The pointer shouldn't be cached.
  network::mojom::URLLoaderFactory* GetURLLoaderFactory();

  // Call |network_factory_.FlushForTesting()|. For test use only.
  void FlushNetworkInterfaceForTesting();

  // Bound with appropriate URLLoaderFactories at HandleFactoryRequests().
  network::mojom::URLLoaderFactoryRequest pending_network_factory_request_;
  network::mojom::URLLoaderFactoryRequest pending_blob_factory_request_;

  // Only accessed on IO thread.
  network::mojom::URLLoaderFactoryPtr network_factory_;
  network::mojom::URLLoaderFactoryPtr blob_factory_;
  network::mojom::URLLoaderFactory* test_factory_ = nullptr;

  // Used to re-create |network_factory_| when connection error happens. Can
  // only be accessed on UI thread. Must be cleared by |StoragePartitionImpl|
  // when it's going away.
  StoragePartitionImpl* partition_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderFactoryGetter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_URL_LOADER_FACTORY_GETTER_H_
