// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_navigation_handle.h"

#include "base/bind.h"
#include "content/browser/service_worker/service_worker_navigation_handle_core.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/browser/browser_thread.h"

namespace content {

ServiceWorkerNavigationHandle::ServiceWorkerNavigationHandle(
    ServiceWorkerContextWrapper* context_wrapper)
    : service_worker_provider_host_id_(kInvalidServiceWorkerProviderId),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  core_ = new ServiceWorkerNavigationHandleCore(weak_factory_.GetWeakPtr(),
                                                context_wrapper);
}

ServiceWorkerNavigationHandle::~ServiceWorkerNavigationHandle() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Delete the ServiceWorkerNavigationHandleCore on the IO thread.
  BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE, core_);
}

void ServiceWorkerNavigationHandle::DidCreateServiceWorkerProviderHost(
    int service_worker_provider_host_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  service_worker_provider_host_id_ = service_worker_provider_host_id;
}

}  // namespace content
