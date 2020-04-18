// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/browser/service_worker/service_worker_quota_client.h"

#include "base/bind.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/public/browser/browser_thread.h"

using blink::mojom::StorageType;
using storage::QuotaClient;

namespace content {
namespace {
void ReportOrigins(QuotaClient::GetOriginsCallback callback,
                   bool restrict_on_host,
                   const std::string host,
                   const std::vector<ServiceWorkerUsageInfo>& usage_info) {
  std::set<url::Origin> origins;
  for (const ServiceWorkerUsageInfo& info : usage_info) {
    if (restrict_on_host && info.origin.host() != host) {
      continue;
    }
    origins.insert(url::Origin::Create(info.origin));
  }
  std::move(callback).Run(origins);
}

void ReportToQuotaStatus(QuotaClient::DeletionCallback callback, bool status) {
  std::move(callback).Run(status ? blink::mojom::QuotaStatusCode::kOk
                                 : blink::mojom::QuotaStatusCode::kUnknown);
}

void FindUsageForOrigin(QuotaClient::GetUsageCallback callback,
                        const GURL& origin,
                        const std::vector<ServiceWorkerUsageInfo>& usage_info) {
  for (const auto& info : usage_info) {
    if (info.origin == origin) {
      std::move(callback).Run(info.total_size_bytes);
      return;
    }
  }
  std::move(callback).Run(0);
}
}  // namespace

ServiceWorkerQuotaClient::ServiceWorkerQuotaClient(
    ServiceWorkerContextWrapper* context)
    : context_(context) {
}

ServiceWorkerQuotaClient::~ServiceWorkerQuotaClient() {
}

QuotaClient::ID ServiceWorkerQuotaClient::id() const {
  return QuotaClient::kServiceWorker;
}

void ServiceWorkerQuotaClient::OnQuotaManagerDestroyed() {
  delete this;
}

void ServiceWorkerQuotaClient::GetOriginUsage(const url::Origin& origin,
                                              StorageType type,
                                              GetUsageCallback callback) {
  if (type != StorageType::kTemporary) {
    std::move(callback).Run(0);
    return;
  }
  context_->GetAllOriginsInfo(base::BindOnce(
      &FindUsageForOrigin, std::move(callback), origin.GetURL()));
}

void ServiceWorkerQuotaClient::GetOriginsForType(StorageType type,
                                                 GetOriginsCallback callback) {
  if (type != StorageType::kTemporary) {
    std::move(callback).Run(std::set<url::Origin>());
    return;
  }
  context_->GetAllOriginsInfo(
      base::BindOnce(&ReportOrigins, std::move(callback), false, ""));
}

void ServiceWorkerQuotaClient::GetOriginsForHost(StorageType type,
                                                 const std::string& host,
                                                 GetOriginsCallback callback) {
  if (type != StorageType::kTemporary) {
    std::move(callback).Run(std::set<url::Origin>());
    return;
  }
  context_->GetAllOriginsInfo(
      base::BindOnce(&ReportOrigins, std::move(callback), true, host));
}

void ServiceWorkerQuotaClient::DeleteOriginData(const url::Origin& origin,
                                                StorageType type,
                                                DeletionCallback callback) {
  if (type != StorageType::kTemporary) {
    std::move(callback).Run(blink::mojom::QuotaStatusCode::kOk);
    return;
  }
  context_->DeleteForOrigin(
      origin.GetURL(),
      base::BindOnce(&ReportToQuotaStatus, std::move(callback)));
}

bool ServiceWorkerQuotaClient::DoesSupport(StorageType type) const {
  return type == StorageType::kTemporary;
}

}  // namespace content
