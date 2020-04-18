// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/appcache/appcache_dispatcher.h"

#include "content/common/appcache.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

AppCacheDispatcher::AppCacheDispatcher(content::AppCacheFrontend* frontend)
    : frontend_(frontend), binding_(this) {}

AppCacheDispatcher::~AppCacheDispatcher() = default;

void AppCacheDispatcher::Bind(mojom::AppCacheFrontendRequest request) {
  binding_.Bind(std::move(request));
}

void AppCacheDispatcher::CacheSelected(int32_t host_id,
                                       mojom::AppCacheInfoPtr info) {
  frontend_->OnCacheSelected(host_id, *info);
}

void AppCacheDispatcher::StatusChanged(const std::vector<int32_t>& host_ids,
                                       AppCacheStatus status) {
  frontend_->OnStatusChanged(host_ids, status);
}

void AppCacheDispatcher::EventRaised(const std::vector<int32_t>& host_ids,
                                     AppCacheEventID event_id) {
  frontend_->OnEventRaised(host_ids, event_id);
}

void AppCacheDispatcher::ProgressEventRaised(
    const std::vector<int32_t>& host_ids,
    const GURL& url,
    int32_t num_total,
    int32_t num_complete) {
  frontend_->OnProgressEventRaised(host_ids, url, num_total, num_complete);
}

void AppCacheDispatcher::ErrorEventRaised(
    const std::vector<int32_t>& host_ids,
    mojom::AppCacheErrorDetailsPtr details) {
  frontend_->OnErrorEventRaised(host_ids, *details);
}

void AppCacheDispatcher::LogMessage(int32_t host_id,
                                    int32_t log_level,
                                    const std::string& message) {
  frontend_->OnLogMessage(
      host_id, static_cast<AppCacheLogLevel>(log_level), message);
}

void AppCacheDispatcher::ContentBlocked(int32_t host_id,
                                        const GURL& manifest_url) {
  frontend_->OnContentBlocked(host_id, manifest_url);
}

void AppCacheDispatcher::SetSubresourceFactory(
    int32_t host_id,
    network::mojom::URLLoaderFactoryPtr url_loader_factory) {
  frontend_->OnSetSubresourceFactory(host_id, std::move(url_loader_factory));
}

}  // namespace content
