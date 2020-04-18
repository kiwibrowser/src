// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/chrome_appcache_service.h"

#include "base/files/file_path.h"
#include "content/browser/appcache/appcache_storage_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/resource_context.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_context_getter.h"
#include "storage/browser/quota/quota_manager.h"

namespace content {

ChromeAppCacheService::ChromeAppCacheService(
    storage::QuotaManagerProxy* quota_manager_proxy)
    : AppCacheServiceImpl(quota_manager_proxy), resource_context_(nullptr) {}

void ChromeAppCacheService::InitializeOnIOThread(
    const base::FilePath& cache_path,
    ResourceContext* resource_context,
    net::URLRequestContextGetter* request_context_getter,
    scoped_refptr<storage::SpecialStoragePolicy> special_storage_policy) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  cache_path_ = cache_path;
  resource_context_ = resource_context;

  // The |request_context_getter| can be NULL in some unit tests.
  //
  // TODO(ajwong): TestProfile is difficult to work with. The
  // SafeBrowsing tests require that GetRequestContext return NULL
  // so we can't depend on having a non-NULL value here. See crbug/149783.
  if (request_context_getter)
    set_request_context(request_context_getter->GetURLRequestContext());

  // Init our base class.
  Initialize(cache_path_);
  set_appcache_policy(this);
  set_special_storage_policy(special_storage_policy.get());
}

void ChromeAppCacheService::Bind(
    std::unique_ptr<mojom::AppCacheBackend> backend,
    mojom::AppCacheBackendRequest request) {
  bindings_.AddBinding(std::move(backend), std::move(request));
}

void ChromeAppCacheService::Shutdown() {
  bindings_.CloseAllBindings();
}

bool ChromeAppCacheService::CanLoadAppCache(const GURL& manifest_url,
                                            const GURL& first_party) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // We don't prompt for read access.
  return GetContentClient()->browser()->AllowAppCache(
      manifest_url, first_party, resource_context_);
}

bool ChromeAppCacheService::CanCreateAppCache(
    const GURL& manifest_url, const GURL& first_party) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return GetContentClient()->browser()->AllowAppCache(
      manifest_url, first_party, resource_context_);
}

ChromeAppCacheService::~ChromeAppCacheService() {}

void ChromeAppCacheService::DeleteOnCorrectThread() const {
  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    delete this;
    return;
  }
  if (BrowserThread::IsThreadInitialized(BrowserThread::IO)) {
    BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE, this);
    return;
  }
  // Better to leak than crash on shutdown.
}

}  // namespace content
