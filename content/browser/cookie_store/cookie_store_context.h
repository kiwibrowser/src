// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COOKIE_STORE_COOKIE_STORE_CONTEXT_H_
#define CONTENT_BROWSER_COOKIE_STORE_COOKIE_STORE_CONTEXT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/memory/scoped_refptr.h"
#include "content/browser/cookie_store/cookie_store_manager.h"
#include "content/common/content_export.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "third_party/blink/public/mojom/cookie_store/cookie_store.mojom.h"
#include "url/origin.h"

namespace content {

class CookieStoreManager;
class ServiceWorkerContextWrapper;

// UI thread handle to a CookieStoreManager.
//
// This class is RefCountedDeleteOnSequence because it has members that must be
// accessed on the IO thread, and therefore must be destroyed on the IO thread.
// Conceptually, CookieStoreContext instances are owned by StoragePartitionImpl.
class CONTENT_EXPORT CookieStoreContext
    : public base::RefCountedDeleteOnSequence<CookieStoreContext> {
 public:
  REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE();

  // Creates an empty CookieStoreContext shell.
  //
  // Newly created instances must be initialized via Initialize() before any
  // other methods are used.
  CookieStoreContext();

  // Creates the underlying CookieStoreManager.
  //
  // This must be called before any other CookieStoreContext method.
  //
  // The newly created CookieStoreManager starts loading any persisted cookie
  // change subscriptions from ServiceWorkerStorage. When the loading completes,
  // the given callback is called with a boolean indicating whether the loading
  // succeeded.
  //
  // It is safe to call all the other methods during the loading operation. This
  // includes creating and using CookieStore mojo services. The
  // CookieStoreManager has well-defined semantics if loading from
  // ServiceWorkerStorage fails, so the caller does not need to handle loading
  // errors.
  void Initialize(
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context,
      base::OnceCallback<void(bool)> callback);

  // Starts listening to cookie changes from a network service instance.
  //
  // The callback is called with the (success / failure) result of subscribing.
  void ListenToCookieChanges(::network::mojom::NetworkContext* network_context,
                             base::OnceCallback<void(bool)> callback);

  // Routes a mojo request to the CookieStoreManager on the IO thread.
  void CreateService(blink::mojom::CookieStoreRequest request,
                     const url::Origin& origin);

 private:
  friend class base::RefCountedDeleteOnSequence<CookieStoreContext>;
  friend class base::DeleteHelper<CookieStoreContext>;
  ~CookieStoreContext();

  void InitializeOnIOThread(
      scoped_refptr<ServiceWorkerContextWrapper> service_worker_context,
      base::OnceCallback<void(bool)> callback);

  void ListenToCookieChangesOnIOThread(
      ::network::mojom::CookieManagerPtrInfo cookie_manager_ptr_info,
      base::OnceCallback<void(bool)> callback);

  void CreateServiceOnIOThread(blink::mojom::CookieStoreRequest request,
                               const url::Origin& origin);

  // Only accessed on the IO thread.
  std::unique_ptr<CookieStoreManager> cookie_store_manager_;

#if DCHECK_IS_ON()
  // Only accesssed on the UI thread.
  bool initialize_called_ = false;
#endif  // DCHECK_IS_ON()

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(CookieStoreContext);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COOKIE_STORE_COOKIE_STORE_CONTEXT_H_
