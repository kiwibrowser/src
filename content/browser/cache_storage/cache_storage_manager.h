// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_MANAGER_H_
#define CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/browser/cache_storage/cache_storage.h"
#include "content/browser/cache_storage/cache_storage_context_impl.h"
#include "content/common/content_export.h"
#include "content/public/browser/cache_storage_context.h"
#include "content/public/browser/cache_storage_usage_info.h"
#include "net/url_request/url_request_context_getter.h"
#include "storage/browser/quota/quota_client.h"
#include "url/origin.h"

namespace base {
class SequencedTaskRunner;
}

namespace storage {
class BlobStorageContext;
class QuotaManagerProxy;
}

namespace content {

class CacheStorageQuotaClient;

namespace cache_storage_manager_unittest {
class CacheStorageManagerTest;
}

enum class CacheStorageOwner {
  kMinValue,

  // Caches that can be accessed by the JS CacheStorage API (developer facing).
  kCacheAPI = kMinValue,

  // Private cache to store background fetch downloads.
  kBackgroundFetch,

  kMaxValue = kBackgroundFetch
};

// Keeps track of a CacheStorage per origin. There is one
// CacheStorageManager per ServiceWorkerContextCore.
// TODO(jkarlin): Remove CacheStorage from memory once they're no
// longer in active use.
class CONTENT_EXPORT CacheStorageManager {
 public:
  static std::unique_ptr<CacheStorageManager> Create(
      const base::FilePath& path,
      scoped_refptr<base::SequencedTaskRunner> cache_task_runner,
      scoped_refptr<storage::QuotaManagerProxy> quota_manager_proxy);

  static std::unique_ptr<CacheStorageManager> Create(
      CacheStorageManager* old_manager);

  // Map a database identifier (computed from an origin) to the path.
  static base::FilePath ConstructOriginPath(const base::FilePath& root_path,
                                            const url::Origin& origin,
                                            CacheStorageOwner owner);

  virtual ~CacheStorageManager();

  // Methods to support the CacheStorage spec. These methods call the
  // corresponding CacheStorage method on the appropriate thread.
  void OpenCache(const url::Origin& origin,
                 CacheStorageOwner owner,
                 const std::string& cache_name,
                 CacheStorage::CacheAndErrorCallback callback);
  void HasCache(const url::Origin& origin,
                CacheStorageOwner owner,
                const std::string& cache_name,
                CacheStorage::BoolAndErrorCallback callback);
  void DeleteCache(const url::Origin& origin,
                   CacheStorageOwner owner,
                   const std::string& cache_name,
                   CacheStorage::ErrorCallback callback);
  void EnumerateCaches(const url::Origin& origin,
                       CacheStorageOwner owner,
                       CacheStorage::IndexCallback callback);
  void MatchCache(const url::Origin& origin,
                  CacheStorageOwner owner,
                  const std::string& cache_name,
                  std::unique_ptr<ServiceWorkerFetchRequest> request,
                  blink::mojom::QueryParamsPtr match_params,
                  CacheStorageCache::ResponseCallback callback);
  void MatchAllCaches(const url::Origin& origin,
                      CacheStorageOwner owner,
                      std::unique_ptr<ServiceWorkerFetchRequest> request,
                      blink::mojom::QueryParamsPtr match_params,
                      CacheStorageCache::ResponseCallback callback);

  // Method to support writing to a cache directly from CacheStorageManager.
  // This should be used by non-CacheAPI owners. The Cache API writes are
  // handled via the dispatcher.
  void WriteToCache(const url::Origin& origin,
                    CacheStorageOwner owner,
                    const std::string& cache_name,
                    std::unique_ptr<ServiceWorkerFetchRequest> request,
                    std::unique_ptr<ServiceWorkerResponse> response,
                    CacheStorage::ErrorCallback callback);

  // This must be called before creating any of the public *Cache functions
  // above.
  void SetBlobParametersForCache(
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      base::WeakPtr<storage::BlobStorageContext> blob_storage_context);

  void AddObserver(CacheStorageContextImpl::Observer* observer);
  void RemoveObserver(CacheStorageContextImpl::Observer* observer);

  void NotifyCacheListChanged(const url::Origin& origin);
  void NotifyCacheContentChanged(const url::Origin& origin,
                                 const std::string& name);

  base::WeakPtr<CacheStorageManager> AsWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  base::FilePath root_path() const { return root_path_; }

 private:
  friend class CacheStorageContextImpl;
  friend class cache_storage_manager_unittest::CacheStorageManagerTest;
  friend class CacheStorageQuotaClient;

  typedef std::map<std::pair<url::Origin, CacheStorageOwner>,
                   std::unique_ptr<CacheStorage>>
      CacheStorageMap;

  CacheStorageManager(
      const base::FilePath& path,
      scoped_refptr<base::SequencedTaskRunner> cache_task_runner,
      scoped_refptr<storage::QuotaManagerProxy> quota_manager_proxy);

  // The returned CacheStorage* is owned by this manager.
  CacheStorage* FindOrCreateCacheStorage(const url::Origin& origin,
                                         CacheStorageOwner owner);

  // QuotaClient and Browsing Data Deletion support
  void GetAllOriginsUsage(CacheStorageOwner owner,
                          CacheStorageContext::GetUsageInfoCallback callback);
  void GetAllOriginsUsageGetSizes(
      std::unique_ptr<std::vector<CacheStorageUsageInfo>> usage_info,
      CacheStorageContext::GetUsageInfoCallback callback);

  void GetOriginUsage(const url::Origin& origin_url,
                      CacheStorageOwner owner,
                      storage::QuotaClient::GetUsageCallback callback);
  void GetOrigins(CacheStorageOwner owner,
                  storage::QuotaClient::GetOriginsCallback callback);
  void GetOriginsForHost(const std::string& host,
                         CacheStorageOwner owner,
                         storage::QuotaClient::GetOriginsCallback callback);
  void DeleteOriginData(const url::Origin& origin,
                        CacheStorageOwner owner,
                        storage::QuotaClient::DeletionCallback callback);
  void DeleteOriginData(const url::Origin& origin, CacheStorageOwner owner);
  void DeleteOriginDidClose(const url::Origin& origin,
                            CacheStorageOwner owner,
                            storage::QuotaClient::DeletionCallback callback,
                            std::unique_ptr<CacheStorage> cache_storage,
                            int64_t origin_size);

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter()
      const {
    return request_context_getter_;
  }

  base::WeakPtr<storage::BlobStorageContext> blob_storage_context() const {
    return blob_context_;
  }

  scoped_refptr<base::SequencedTaskRunner> cache_task_runner() const {
    return cache_task_runner_;
  }

  bool IsMemoryBacked() const { return root_path_.empty(); }

  base::FilePath root_path_;
  scoped_refptr<base::SequencedTaskRunner> cache_task_runner_;

  scoped_refptr<storage::QuotaManagerProxy> quota_manager_proxy_;

  // The map owns the CacheStorages and the CacheStorages are only accessed on
  // |cache_task_runner_|.
  CacheStorageMap cache_storage_map_;

  base::ObserverList<CacheStorageContextImpl::Observer> observers_;

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  base::WeakPtr<storage::BlobStorageContext> blob_context_;

  base::WeakPtrFactory<CacheStorageManager> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(CacheStorageManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_MANAGER_H_
