// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_dispatcher_host.h"

#include <map>
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "content/browser/appcache/appcache_navigation_handle_core.h"
#include "content/browser/appcache/chrome_appcache_service.h"
#include "content/browser/bad_message.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/browser_side_navigation_policy.h"

namespace content {

AppCacheDispatcherHost::AppCacheDispatcherHost(
    ChromeAppCacheService* appcache_service,
    int process_id)
    : appcache_service_(appcache_service),
      frontend_proxy_(process_id),
      weak_factory_(this) {
  if (appcache_service) {
    backend_impl_.Initialize(appcache_service, &frontend_proxy_, process_id);
  }
}

AppCacheDispatcherHost::~AppCacheDispatcherHost() = default;

// static
void AppCacheDispatcherHost::Create(ChromeAppCacheService* appcache_service,
                                    int process_id,
                                    mojom::AppCacheBackendRequest request) {
  appcache_service->Bind(
      std::make_unique<AppCacheDispatcherHost>(appcache_service, process_id),
      std::move(request));
}

void AppCacheDispatcherHost::RegisterHost(int32_t host_id) {
  if (appcache_service_) {
    // PlzNavigate
    // The AppCacheHost could have been precreated in which case we want to
    // register it with the backend here.
    if (IsBrowserSideNavigationEnabled()) {
      std::unique_ptr<content::AppCacheHost> host =
          AppCacheNavigationHandleCore::GetPrecreatedHost(host_id);
      if (host.get()) {
        backend_impl_.RegisterPrecreatedHost(std::move(host));
        return;
      }
    }
    if (!backend_impl_.RegisterHost(host_id)) {
      mojo::ReportBadMessage("ACDH_REGISTER");
    }
  }
}

void AppCacheDispatcherHost::UnregisterHost(int32_t host_id) {
  if (appcache_service_) {
    if (!backend_impl_.UnregisterHost(host_id)) {
      mojo::ReportBadMessage("ACDH_UNREGISTER");
    }
  }
}

void AppCacheDispatcherHost::SetSpawningHostId(int32_t host_id,
                                               int spawning_host_id) {
  if (appcache_service_) {
    if (!backend_impl_.SetSpawningHostId(host_id, spawning_host_id))
      mojo::ReportBadMessage("ACDH_SET_SPAWNING");
  }
}

void AppCacheDispatcherHost::SelectCache(int32_t host_id,
                                         const GURL& document_url,
                                         int64_t cache_document_was_loaded_from,
                                         const GURL& opt_manifest_url) {
  if (appcache_service_) {
    if (!backend_impl_.SelectCache(host_id, document_url,
                                   cache_document_was_loaded_from,
                                   opt_manifest_url)) {
      mojo::ReportBadMessage("ACDH_SELECT_CACHE");
    }
  } else {
    frontend_proxy_.OnCacheSelected(host_id, std::move(AppCacheInfo()));
  }
}

void AppCacheDispatcherHost::SelectCacheForSharedWorker(int32_t host_id,
                                                        int64_t appcache_id) {
  if (appcache_service_) {
    if (!backend_impl_.SelectCacheForSharedWorker(host_id, appcache_id))
      mojo::ReportBadMessage("ACDH_SELECT_CACHE_FOR_SHARED_WORKER");
  } else {
    frontend_proxy_.OnCacheSelected(host_id, std::move(AppCacheInfo()));
  }
}

void AppCacheDispatcherHost::MarkAsForeignEntry(
    int32_t host_id,
    const GURL& document_url,
    int64_t cache_document_was_loaded_from) {
  if (appcache_service_) {
    if (!backend_impl_.MarkAsForeignEntry(host_id, document_url,
                                          cache_document_was_loaded_from)) {
      mojo::ReportBadMessage("ACDH_MARK_AS_FOREIGN_ENTRY");
    }
  }
}

void AppCacheDispatcherHost::GetResourceList(int32_t host_id,
                                             GetResourceListCallback callback) {
  std::vector<AppCacheResourceInfo> params;
  std::vector<mojom::AppCacheResourceInfoPtr> out;
  if (appcache_service_) {
    backend_impl_.GetResourceList(host_id, &params);

    // Box up params for output.
    out.reserve(params.size());
    for (auto& p : params) {
      out.emplace_back(base::in_place, std::move(p));
    }
  }
  std::move(callback).Run(std::move(out));
}

void AppCacheDispatcherHost::GetStatus(int32_t host_id,
                                       GetStatusCallback callback) {
  if (appcache_service_) {
    if (backend_impl_.GetStatusWithCallback(host_id, &callback)) {
      return;
    } else {
      mojo::ReportBadMessage("ACDH_GET_STATUS");
    }
  }
  std::move(callback).Run(AppCacheStatus::APPCACHE_STATUS_UNCACHED);
}

void AppCacheDispatcherHost::StartUpdate(int32_t host_id,
                                         StartUpdateCallback callback) {
  if (appcache_service_) {
    if (backend_impl_.StartUpdateWithCallback(host_id, &callback)) {
      return;
    } else {
      mojo::ReportBadMessage("ACDH_START_UPDATE");
    }
  }
  std::move(callback).Run(false);
}

void AppCacheDispatcherHost::SwapCache(int32_t host_id,
                                       SwapCacheCallback callback) {
  if (appcache_service_) {
    if (backend_impl_.SwapCacheWithCallback(host_id, &callback)) {
      return;
    } else {
      mojo::ReportBadMessage("ACDH_SWAP_CACHE");
    }
  }
  std::move(callback).Run(false);
}

}  // namespace content
