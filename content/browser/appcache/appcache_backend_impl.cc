// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_backend_impl.h"

#include "content/browser/appcache/appcache.h"
#include "content/browser/appcache/appcache_group.h"
#include "content/browser/appcache/appcache_service_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/browser_side_navigation_policy.h"

namespace content {

AppCacheBackendImpl::AppCacheBackendImpl()
    : service_(nullptr), frontend_(nullptr), process_id_(0) {}

AppCacheBackendImpl::~AppCacheBackendImpl() {
  hosts_.clear();
  if (service_)
    service_->UnregisterBackend(this);
}

void AppCacheBackendImpl::Initialize(AppCacheServiceImpl* service,
                                     AppCacheFrontend* frontend,
                                     int process_id) {
  DCHECK(!service_ && !frontend_ && frontend && service);
  service_ = service;
  frontend_ = frontend;
  process_id_ = process_id;
  service_->RegisterBackend(this);
}

bool AppCacheBackendImpl::RegisterHost(int id) {
  if (GetHost(id))
    return false;

  hosts_[id] = std::make_unique<AppCacheHost>(id, frontend_, service_);
  return true;
}

bool AppCacheBackendImpl::UnregisterHost(int id) {
  return hosts_.erase(id) > 0;
}

bool AppCacheBackendImpl::SetSpawningHostId(
    int host_id,
    int spawning_host_id) {
  AppCacheHost* host = GetHost(host_id);
  if (!host)
    return false;
  host->SetSpawningHostId(process_id_, spawning_host_id);
  return true;
}

bool AppCacheBackendImpl::SelectCache(
    int host_id,
    const GURL& document_url,
    const int64_t cache_document_was_loaded_from,
    const GURL& manifest_url) {
  AppCacheHost* host = GetHost(host_id);
  if (!host)
    return false;

  return host->SelectCache(document_url, cache_document_was_loaded_from,
                    manifest_url);
}

bool AppCacheBackendImpl::SelectCacheForSharedWorker(int host_id,
                                                     int64_t appcache_id) {
  AppCacheHost* host = GetHost(host_id);
  if (!host)
    return false;

  return host->SelectCacheForSharedWorker(appcache_id);
}

bool AppCacheBackendImpl::MarkAsForeignEntry(
    int host_id,
    const GURL& document_url,
    int64_t cache_document_was_loaded_from) {
  AppCacheHost* host = GetHost(host_id);
  if (!host)
    return false;

  return host->MarkAsForeignEntry(document_url, cache_document_was_loaded_from);
}

bool AppCacheBackendImpl::GetStatusWithCallback(int host_id,
                                                GetStatusCallback* callback) {
  AppCacheHost* host = GetHost(host_id);
  if (!host)
    return false;

  host->GetStatusWithCallback(std::move(*callback));
  return true;
}

bool AppCacheBackendImpl::StartUpdateWithCallback(
    int host_id,
    StartUpdateCallback* callback) {
  AppCacheHost* host = GetHost(host_id);
  if (!host)
    return false;

  host->StartUpdateWithCallback(std::move(*callback));
  return true;
}

bool AppCacheBackendImpl::SwapCacheWithCallback(int host_id,
                                                SwapCacheCallback* callback) {
  AppCacheHost* host = GetHost(host_id);
  if (!host)
    return false;

  host->SwapCacheWithCallback(std::move(*callback));
  return true;
}

void AppCacheBackendImpl::GetResourceList(
    int host_id, std::vector<AppCacheResourceInfo>* resource_infos) {
  AppCacheHost* host = GetHost(host_id);
  if (!host)
    return;

  host->GetResourceList(resource_infos);
}

std::unique_ptr<AppCacheHost> AppCacheBackendImpl::TransferHostOut(
    int host_id) {
  auto found = hosts_.find(host_id);
  if (found == hosts_.end()) {
    NOTREACHED();
    return std::unique_ptr<AppCacheHost>();
  }

  std::unique_ptr<AppCacheHost> transferree = std::move(found->second);

  // Put a new empty host in its place.
  found->second = std::make_unique<AppCacheHost>(host_id, frontend_, service_);

  // We give up ownership.
  transferree->PrepareForTransfer();
  return transferree;
}

void AppCacheBackendImpl::TransferHostIn(int new_host_id,
                                         std::unique_ptr<AppCacheHost> host) {
  auto found = hosts_.find(new_host_id);
  if (found == hosts_.end()) {
    NOTREACHED();
    return;
  }

  host->CompleteTransfer(new_host_id, frontend_);
  found->second = std::move(host);
}

void AppCacheBackendImpl::RegisterPrecreatedHost(
    std::unique_ptr<AppCacheHost> host) {
  DCHECK(IsBrowserSideNavigationEnabled());
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DCHECK(host.get());
  DCHECK(hosts_.find(host->host_id()) == hosts_.end());
  // Switch the frontend proxy so that the host can make IPC calls from
  // here on.
  host->set_frontend(frontend_);
  hosts_[host->host_id()] = std::move(host);
}

}  // namespace content
