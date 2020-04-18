// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_DISPATCHER_HOST_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/process/process.h"
#include "content/browser/appcache/appcache_backend_impl.h"
#include "content/browser/appcache/appcache_frontend_proxy.h"
#include "content/common/appcache.mojom.h"
#include "content/public/browser/browser_message_filter.h"

namespace content {
class ChromeAppCacheService;

// Handles appcache related messages sent to the main browser process from
// its child processes. There is a distinct host for each child process.
// Messages are handled on the IO thread. The RenderProcessHostImpl creates
// an instance and delegates calls to it.
class AppCacheDispatcherHost : public mojom::AppCacheBackend {
 public:
  AppCacheDispatcherHost(ChromeAppCacheService* appcache_service,
                         int process_id);
  ~AppCacheDispatcherHost() override;

  static void Create(ChromeAppCacheService* appcache_service,
                     int process_id,
                     mojom::AppCacheBackendRequest request);

 private:
  // mojom::AppCacheHost
  void RegisterHost(int32_t host_id) override;
  void UnregisterHost(int32_t host_id) override;
  void SetSpawningHostId(int32_t host_id, int spawning_host_id) override;
  void SelectCache(int32_t host_id,
                   const GURL& document_url,
                   int64_t cache_document_was_loaded_from,
                   const GURL& opt_manifest_url) override;
  void SelectCacheForSharedWorker(int32_t host_id,
                                  int64_t appcache_id) override;
  void MarkAsForeignEntry(int32_t host_id,
                          const GURL& document_url,
                          int64_t cache_document_was_loaded_from) override;
  void GetStatus(int32_t host_id, GetStatusCallback callback) override;
  void StartUpdate(int32_t host_id, StartUpdateCallback callback) override;
  void SwapCache(int32_t host_id, SwapCacheCallback callback) override;
  void GetResourceList(int32_t host_id,
                       GetResourceListCallback callback) override;

  // This object is owned by the |ChromeAppCacheService|, so this is safe.
  ChromeAppCacheService* appcache_service_;
  AppCacheFrontendProxy frontend_proxy_;
  AppCacheBackendImpl backend_impl_;

  base::WeakPtrFactory<AppCacheDispatcherHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppCacheDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_DISPATCHER_HOST_H_
