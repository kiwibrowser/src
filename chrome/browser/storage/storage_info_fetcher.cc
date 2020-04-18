// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/storage/storage_info_fetcher.h"

#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "storage/browser/quota/quota_manager.h"

using content::BrowserContext;
using content::BrowserThread;

StorageInfoFetcher::StorageInfoFetcher(Profile* profile) {
  quota_manager_ = content::BrowserContext::GetDefaultStoragePartition(
      profile)->GetQuotaManager();
}

StorageInfoFetcher::~StorageInfoFetcher() {
}

void StorageInfoFetcher::FetchStorageInfo(const FetchCallback& fetch_callback) {
  // Balanced in OnFetchCompleted.
  AddRef();

  fetch_callback_ = fetch_callback;

  // QuotaManager must be called on IO thread, but the callback must then be
  // called on the UI thread.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&StorageInfoFetcher::GetUsageInfo, this,
          base::Bind(&StorageInfoFetcher::OnGetUsageInfoInternal, this)));
}

void StorageInfoFetcher::ClearStorage(const std::string& host,
                                      blink::mojom::StorageType type,
                                      const ClearCallback& clear_callback) {
  // Balanced in OnUsageCleared.
  AddRef();

  clear_callback_ = clear_callback;
  type_to_delete_ = type;

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&storage::QuotaManager::DeleteHostData,
                 quota_manager_,
                 host,
                 type,
                 storage::QuotaClient::kAllClientsMask,
                 base::Bind(&StorageInfoFetcher::OnUsageClearedInternal,
                            this)));
}

void StorageInfoFetcher::GetUsageInfo(storage::GetUsageInfoCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  quota_manager_->GetUsageInfo(std::move(callback));
}

void StorageInfoFetcher::OnGetUsageInfoInternal(
    const storage::UsageInfoEntries& entries) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  entries_.insert(entries_.begin(), entries.begin(), entries.end());
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&StorageInfoFetcher::OnFetchCompleted, this));
}

void StorageInfoFetcher::OnFetchCompleted() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  fetch_callback_.Run(entries_);

  Release();
}

void StorageInfoFetcher::OnUsageClearedInternal(
    blink::mojom::QuotaStatusCode code) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  quota_manager_->ResetUsageTracker(type_to_delete_);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&StorageInfoFetcher::OnClearCompleted, this, code));
}

void StorageInfoFetcher::OnClearCompleted(blink::mojom::QuotaStatusCode code) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  clear_callback_.Run(code);

  Release();
}
