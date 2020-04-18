// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/test/mock_quota_manager.h"

#include <limits>
#include <memory>

#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "url/gurl.h"

namespace content {

MockQuotaManager::OriginInfo::OriginInfo(
    const GURL& origin,
    StorageType type,
    int quota_client_mask,
    base::Time modified)
    : origin(origin),
      type(type),
      quota_client_mask(quota_client_mask),
      modified(modified) {
}

MockQuotaManager::OriginInfo::~OriginInfo() = default;

MockQuotaManager::StorageInfo::StorageInfo()
    : usage(0), quota(std::numeric_limits<int64_t>::max()) {}
MockQuotaManager::StorageInfo::~StorageInfo() = default;

MockQuotaManager::MockQuotaManager(
    bool is_incognito,
    const base::FilePath& profile_path,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_thread,
    const scoped_refptr<SpecialStoragePolicy>& special_storage_policy)
    : QuotaManager(is_incognito,
                   profile_path,
                   io_thread,
                   special_storage_policy,
                   storage::GetQuotaSettingsFunc()),
      weak_factory_(this) {}

void MockQuotaManager::GetUsageAndQuota(const GURL& origin,
                                        StorageType type,
                                        UsageAndQuotaCallback callback) {
  StorageInfo& info = usage_and_quota_map_[std::make_pair(origin, type)];
  std::move(callback).Run(blink::mojom::QuotaStatusCode::kOk, info.usage,
                          info.quota);
}

void MockQuotaManager::SetQuota(const GURL& origin,
                                StorageType type,
                                int64_t quota) {
  usage_and_quota_map_[std::make_pair(origin, type)].quota = quota;
}

bool MockQuotaManager::AddOrigin(
    const GURL& origin,
    StorageType type,
    int quota_client_mask,
    base::Time modified) {
  origins_.push_back(OriginInfo(origin, type, quota_client_mask, modified));
  return true;
}

bool MockQuotaManager::OriginHasData(
    const GURL& origin,
    StorageType type,
    QuotaClient::ID quota_client) const {
  for (std::vector<OriginInfo>::const_iterator current = origins_.begin();
       current != origins_.end();
       ++current) {
    if (current->origin == origin &&
        current->type == type &&
        current->quota_client_mask & quota_client)
      return true;
  }
  return false;
}

void MockQuotaManager::GetOriginsModifiedSince(StorageType type,
                                               base::Time modified_since,
                                               GetOriginsCallback callback) {
  std::set<GURL>* origins_to_return = new std::set<GURL>();
  for (std::vector<OriginInfo>::const_iterator current = origins_.begin();
       current != origins_.end();
       ++current) {
    if (current->type == type && current->modified >= modified_since)
      origins_to_return->insert(current->origin);
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MockQuotaManager::DidGetModifiedSince,
                                weak_factory_.GetWeakPtr(), std::move(callback),
                                base::Owned(origins_to_return), type));
}

void MockQuotaManager::DeleteOriginData(const GURL& origin,
                                        StorageType type,
                                        int quota_client_mask,
                                        StatusCallback callback) {
  for (std::vector<OriginInfo>::iterator current = origins_.begin();
       current != origins_.end();
       ++current) {
    if (current->origin == origin && current->type == type) {
      // Modify the mask: if it's 0 after "deletion", remove the origin.
      current->quota_client_mask &= ~quota_client_mask;
      if (current->quota_client_mask == 0)
        origins_.erase(current);
      break;
    }
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&MockQuotaManager::DidDeleteOriginData,
                                weak_factory_.GetWeakPtr(), std::move(callback),
                                blink::mojom::QuotaStatusCode::kOk));
}

MockQuotaManager::~MockQuotaManager() = default;

void MockQuotaManager::UpdateUsage(const GURL& origin,
                                   StorageType type,
                                   int64_t delta) {
  usage_and_quota_map_[std::make_pair(origin, type)].usage += delta;
}

void MockQuotaManager::DidGetModifiedSince(GetOriginsCallback callback,
                                           std::set<GURL>* origins,
                                           StorageType storage_type) {
  std::move(callback).Run(*origins, storage_type);
}

void MockQuotaManager::DidDeleteOriginData(
    StatusCallback callback,
    blink::mojom::QuotaStatusCode status) {
  std::move(callback).Run(status);
}

}  // namespace content
