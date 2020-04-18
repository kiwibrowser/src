// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_quota_client.h"

#include <algorithm>
#include <map>
#include <set>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "content/browser/appcache/appcache_service_impl.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"

using blink::mojom::StorageType;
using storage::QuotaClient;

namespace {
blink::mojom::QuotaStatusCode NetErrorCodeToQuotaStatus(int code) {
  if (code == net::OK)
    return blink::mojom::QuotaStatusCode::kOk;
  else if (code == net::ERR_ABORTED)
    return blink::mojom::QuotaStatusCode::kErrorAbort;
  else
    return blink::mojom::QuotaStatusCode::kUnknown;
}

void RunFront(content::AppCacheQuotaClient::RequestQueue* queue) {
  base::OnceClosure request = std::move(queue->front());
  queue->pop_front();
  std::move(request).Run();
}
}  // namespace

namespace content {

AppCacheQuotaClient::AppCacheQuotaClient(AppCacheServiceImpl* service)
    : service_(service),
      appcache_is_ready_(false),
      quota_manager_is_destroyed_(false) {
}

AppCacheQuotaClient::~AppCacheQuotaClient() {
  DCHECK(pending_batch_requests_.empty());
  DCHECK(pending_serial_requests_.empty());
  DCHECK(current_delete_request_callback_.is_null());
}

QuotaClient::ID AppCacheQuotaClient::id() const {
  return kAppcache;
}

void AppCacheQuotaClient::OnQuotaManagerDestroyed() {
  DeletePendingRequests();
  if (!current_delete_request_callback_.is_null()) {
    current_delete_request_callback_.Reset();
    GetServiceDeleteCallback()->Cancel();
  }

  quota_manager_is_destroyed_ = true;
  if (!service_)
    delete this;
}

void AppCacheQuotaClient::GetOriginUsage(const url::Origin& origin,
                                         StorageType type,
                                         GetUsageCallback callback) {
  DCHECK(!callback.is_null());
  DCHECK(!quota_manager_is_destroyed_);

  if (!service_) {
    std::move(callback).Run(0);
    return;
  }

  if (!appcache_is_ready_) {
    pending_batch_requests_.push_back(base::BindOnce(
        &AppCacheQuotaClient::GetOriginUsage, base::Unretained(this), origin,
        type, std::move(callback)));
    return;
  }

  if (type != StorageType::kTemporary) {
    std::move(callback).Run(0);
    return;
  }

  const AppCacheStorage::UsageMap* map = GetUsageMap();
  auto found = map->find(origin);
  if (found == map->end()) {
    std::move(callback).Run(0);
    return;
  }
  std::move(callback).Run(found->second);
}

void AppCacheQuotaClient::GetOriginsForType(StorageType type,
                                            GetOriginsCallback callback) {
  GetOriginsHelper(type, std::string(), std::move(callback));
}

void AppCacheQuotaClient::GetOriginsForHost(StorageType type,
                                            const std::string& host,
                                            GetOriginsCallback callback) {
  DCHECK(!callback.is_null());
  if (host.empty()) {
    std::move(callback).Run(std::set<url::Origin>());
    return;
  }
  GetOriginsHelper(type, host, std::move(callback));
}

void AppCacheQuotaClient::DeleteOriginData(const url::Origin& origin,
                                           StorageType type,
                                           DeletionCallback callback) {
  DCHECK(!quota_manager_is_destroyed_);

  if (!service_) {
    std::move(callback).Run(blink::mojom::QuotaStatusCode::kErrorAbort);
    return;
  }

  if (!appcache_is_ready_ || !current_delete_request_callback_.is_null()) {
    pending_serial_requests_.push_back(base::BindOnce(
        &AppCacheQuotaClient::DeleteOriginData, base::Unretained(this), origin,
        type, std::move(callback)));
    return;
  }

  current_delete_request_callback_ = std::move(callback);
  if (type != StorageType::kTemporary) {
    DidDeleteAppCachesForOrigin(net::OK);
    return;
  }

  service_->DeleteAppCachesForOrigin(origin,
                                     GetServiceDeleteCallback()->callback());
}

bool AppCacheQuotaClient::DoesSupport(StorageType type) const {
  return type == StorageType::kTemporary;
}

void AppCacheQuotaClient::DidDeleteAppCachesForOrigin(int rv) {
  DCHECK(service_);
  if (quota_manager_is_destroyed_)
    return;

  // Finish the request by calling our callers callback.
  std::move(current_delete_request_callback_)
      .Run(NetErrorCodeToQuotaStatus(rv));
  if (pending_serial_requests_.empty())
    return;

  // Start the next in the queue.
  RunFront(&pending_serial_requests_);
}

void AppCacheQuotaClient::GetOriginsHelper(StorageType type,
                                           const std::string& opt_host,
                                           GetOriginsCallback callback) {
  DCHECK(!callback.is_null());
  DCHECK(!quota_manager_is_destroyed_);

  if (!service_) {
    std::move(callback).Run(std::set<url::Origin>());
    return;
  }

  if (!appcache_is_ready_) {
    pending_batch_requests_.push_back(base::BindOnce(
        &AppCacheQuotaClient::GetOriginsHelper, base::Unretained(this), type,
        opt_host, std::move(callback)));
    return;
  }

  if (type != StorageType::kTemporary) {
    std::move(callback).Run(std::set<url::Origin>());
    return;
  }

  std::set<url::Origin> origins;
  for (const auto& pair : *GetUsageMap()) {
    if (opt_host.empty() || pair.first.host() == opt_host)
      origins.insert(pair.first);
  }
  std::move(callback).Run(origins);
}

void AppCacheQuotaClient::ProcessPendingRequests() {
  DCHECK(appcache_is_ready_);
  while (!pending_batch_requests_.empty())
    RunFront(&pending_batch_requests_);

  if (!pending_serial_requests_.empty())
    RunFront(&pending_serial_requests_);
}

void AppCacheQuotaClient::DeletePendingRequests() {
  pending_batch_requests_.clear();
  pending_serial_requests_.clear();
}

const AppCacheStorage::UsageMap* AppCacheQuotaClient::GetUsageMap() {
  DCHECK(service_);
  return service_->storage()->usage_map();
}

net::CancelableCompletionCallback*
AppCacheQuotaClient::GetServiceDeleteCallback() {
  // Lazily created due to CancelableCompletionCallback's threading
  // restrictions, there is no way to detach from the thread created on.
  if (!service_delete_callback_) {
    service_delete_callback_.reset(
        new net::CancelableCompletionCallback(
            base::Bind(&AppCacheQuotaClient::DidDeleteAppCachesForOrigin,
                       base::Unretained(this))));
  }
  return service_delete_callback_.get();
}

void AppCacheQuotaClient::NotifyAppCacheReady() {
  // Can reoccur during reinitialization.
  if (!appcache_is_ready_) {
    appcache_is_ready_ = true;
    ProcessPendingRequests();
  }
}

void AppCacheQuotaClient::NotifyAppCacheDestroyed() {
  service_ = nullptr;
  while (!pending_batch_requests_.empty())
    RunFront(&pending_batch_requests_);

  while (!pending_serial_requests_.empty())
    RunFront(&pending_serial_requests_);

  if (!current_delete_request_callback_.is_null()) {
    std::move(current_delete_request_callback_)
        .Run(blink::mojom::QuotaStatusCode::kErrorAbort);
    GetServiceDeleteCallback()->Cancel();
  }

  if (quota_manager_is_destroyed_)
    delete this;
}

}  // namespace content
