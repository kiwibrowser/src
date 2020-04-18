// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_H_
#define CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/cache_storage/cache_storage_cache.h"
#include "content/browser/cache_storage/cache_storage_cache_observer.h"
#include "third_party/blink/public/platform/modules/cache_storage/cache_storage.mojom.h"
#include "url/origin.h"

namespace base {
class SequencedTaskRunner;
}

namespace net {
class URLRequestContextGetter;
}

namespace storage {
class BlobStorageContext;
}

namespace content {
class CacheStorageCacheHandle;
class CacheStorageIndex;
class CacheStorageManager;
class CacheStorageScheduler;
enum class CacheStorageOwner;

namespace cache_storage_manager_unittest {
class CacheStorageManagerTest;
FORWARD_DECLARE_TEST(CacheStorageManagerTest, PersistedCacheKeyUsed);
FORWARD_DECLARE_TEST(CacheStorageManagerTest, TestErrorInitializingCache);
}  // namespace cache_storage_manager_unittest

// TODO(jkarlin): Constrain the total bytes used per origin.

// CacheStorage holds the set of caches for a given origin. It is
// owned by the CacheStorageManager. This class expects to be run
// on the IO thread. The asynchronous methods are executed serially.
class CONTENT_EXPORT CacheStorage : public CacheStorageCacheObserver {
 public:
  constexpr static int64_t kSizeUnknown = -1;

  using BoolAndErrorCallback =
      base::OnceCallback<void(bool, blink::mojom::CacheStorageError)>;
  using ErrorCallback =
      base::OnceCallback<void(blink::mojom::CacheStorageError)>;
  using CacheAndErrorCallback =
      base::OnceCallback<void(CacheStorageCacheHandle,
                              blink::mojom::CacheStorageError)>;
  using IndexCallback = base::OnceCallback<void(const CacheStorageIndex&)>;
  using SizeCallback = base::OnceCallback<void(int64_t)>;

  static const char kIndexFileName[];

  CacheStorage(
      const base::FilePath& origin_path,
      bool memory_only,
      base::SequencedTaskRunner* cache_task_runner,
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      scoped_refptr<storage::QuotaManagerProxy> quota_manager_proxy,
      base::WeakPtr<storage::BlobStorageContext> blob_context,
      CacheStorageManager* cache_storage_manager,
      const url::Origin& origin,
      CacheStorageOwner owner);

  // Any unfinished asynchronous operations may not complete or call their
  // callbacks.
  virtual ~CacheStorage();

  // Get the cache for the given key. If the cache is not found it is
  // created. The CacheStorgeCacheHandle in the callback prolongs the lifetime
  // of the cache. Once all handles to a cache are deleted the cache is deleted.
  // The cache will also be deleted in the CacheStorage's destructor so be sure
  // to check the handle's value before using it.
  void OpenCache(const std::string& cache_name, CacheAndErrorCallback callback);

  // Calls the callback with whether or not the cache exists.
  void HasCache(const std::string& cache_name, BoolAndErrorCallback callback);

  // Deletes the cache if it exists. If it doesn't exist,
  // blink::mojom::CacheStorageError::kErrorNotFound is returned. Any
  // existing CacheStorageCacheHandle(s) to the cache will remain valid but
  // future CacheStorage operations won't be able to access the cache. The cache
  // isn't actually erased from disk until the last handle is dropped.
  void DoomCache(const std::string& cache_name, ErrorCallback callback);

  // Calls the callback with the cache index.
  void EnumerateCaches(IndexCallback callback);

  // Calls match on the cache with the given |cache_name|.
  void MatchCache(const std::string& cache_name,
                  std::unique_ptr<ServiceWorkerFetchRequest> request,
                  blink::mojom::QueryParamsPtr match_params,
                  CacheStorageCache::ResponseCallback callback);

  // Calls match on all of the caches in parallel, calling |callback| with the
  // response from the first cache (in order of cache creation) to have the
  // entry. If no response is found then |callback| is called with
  // blink::mojom::CacheStorageError::kErrorNotFound.
  void MatchAllCaches(std::unique_ptr<ServiceWorkerFetchRequest> request,
                      blink::mojom::QueryParamsPtr match_params,
                      CacheStorageCache::ResponseCallback callback);

  // Puts the request/response pair in the cache.
  void WriteToCache(const std::string& cache_name,
                    std::unique_ptr<ServiceWorkerFetchRequest> request,
                    std::unique_ptr<ServiceWorkerResponse> response,
                    CacheStorage::ErrorCallback callback);

  // Sums the sizes of each cache and closes them. Runs |callback| with the
  // size. The sizes include any doomed caches and will also force close all
  // caches even if there are existing handles to them.
  void GetSizeThenCloseAllCaches(SizeCallback callback);

  // The size of all of the origin's contents. This value should be used as an
  // estimate only since the cache may be modified at any time.
  void Size(SizeCallback callback);

  // The functions below are for tests to verify that the operations run
  // serially.
  void StartAsyncOperationForTesting();
  void CompleteAsyncOperationForTesting();

  // Removes the manager reference. Called before this storage is deleted by the
  // manager, since it is removed from manager's storage map before deleting.
  void ResetManager();

  // CacheStorageCacheObserver:
  void CacheSizeUpdated(const CacheStorageCache* cache) override;

 private:
  friend class CacheStorageCacheHandle;
  friend class CacheStorageCache;
  friend class cache_storage_manager_unittest::CacheStorageManagerTest;
  FRIEND_TEST_ALL_PREFIXES(
      cache_storage_manager_unittest::CacheStorageManagerTest,
      PersistedCacheKeyUsed);
  FRIEND_TEST_ALL_PREFIXES(
      cache_storage_manager_unittest::CacheStorageManagerTest,
      TestErrorInitializingCache);
  class CacheLoader;
  class MemoryLoader;
  class SimpleCacheLoader;
  struct CacheMatchResponse;

  typedef std::map<std::string, std::unique_ptr<CacheStorageCache>> CacheMap;

  // Generate a new padding key. For testing only and *not thread safe*.
  static void GenerateNewKeyForTesting();

  // Functions for exposing handles to CacheStorageCache to clients.
  CacheStorageCacheHandle CreateCacheHandle(CacheStorageCache* cache);
  void AddCacheHandleRef(CacheStorageCache* cache);
  void DropCacheHandleRef(CacheStorageCache* cache);

  // Returns a CacheStorageCacheHandle for the given name if the name is known.
  // If the CacheStorageCache has been deleted, creates a new one.
  CacheStorageCacheHandle GetLoadedCache(const std::string& cache_name);

  // Initializer and its callback are below.
  void LazyInit();
  void LazyInitImpl();
  void LazyInitDidLoadIndex(std::unique_ptr<CacheStorageIndex> index);

  // The Open and CreateCache callbacks are below.
  void OpenCacheImpl(const std::string& cache_name,
                     CacheAndErrorCallback callback);
  void CreateCacheDidCreateCache(const std::string& cache_name,
                                 CacheAndErrorCallback callback,
                                 std::unique_ptr<CacheStorageCache> cache);
  void CreateCacheDidWriteIndex(CacheAndErrorCallback callback,
                                CacheStorageCacheHandle cache_handle,
                                bool success);

  // The HasCache callbacks are below.
  void HasCacheImpl(const std::string& cache_name,
                    BoolAndErrorCallback callback);

  // The DeleteCache callbacks are below.
  void DoomCacheImpl(const std::string& cache_name, ErrorCallback callback);
  void DeleteCacheDidWriteIndex(CacheStorageCacheHandle cache_handle,
                                ErrorCallback callback,
                                bool success);
  void DeleteCacheFinalize(CacheStorageCache* doomed_cache);
  void DeleteCacheDidGetSize(CacheStorageCache* doomed_cache,
                             int64_t cache_size);
  void DeleteCacheDidCleanUp(bool success);

  // The EnumerateCache callbacks are below.
  void EnumerateCachesImpl(IndexCallback callback);

  // The MatchCache callbacks are below.
  void MatchCacheImpl(const std::string& cache_name,
                      std::unique_ptr<ServiceWorkerFetchRequest> request,
                      blink::mojom::QueryParamsPtr match_params,
                      CacheStorageCache::ResponseCallback callback);
  void MatchCacheDidMatch(CacheStorageCacheHandle cache_handle,
                          CacheStorageCache::ResponseCallback callback,
                          blink::mojom::CacheStorageError error,
                          std::unique_ptr<ServiceWorkerResponse> response);

  // The MatchAllCaches callbacks are below.
  void MatchAllCachesImpl(std::unique_ptr<ServiceWorkerFetchRequest> request,
                          blink::mojom::QueryParamsPtr match_params,
                          CacheStorageCache::ResponseCallback callback);
  void MatchAllCachesDidMatch(
      CacheStorageCacheHandle cache_handle,
      CacheMatchResponse* out_match_response,
      const base::RepeatingClosure& barrier_closure,
      blink::mojom::CacheStorageError error,
      std::unique_ptr<ServiceWorkerResponse> service_worker_response);
  void MatchAllCachesDidMatchAll(
      std::unique_ptr<std::vector<CacheMatchResponse>> match_responses,
      CacheStorageCache::ResponseCallback callback);

  // WriteToCache callbacks.
  void WriteToCacheImpl(const std::string& cache_name,
                        std::unique_ptr<ServiceWorkerFetchRequest> request,
                        std::unique_ptr<ServiceWorkerResponse> response,
                        CacheStorage::ErrorCallback callback);

  void GetSizeThenCloseAllCachesImpl(SizeCallback callback);

  void SizeImpl(SizeCallback callback);
  void SizeRetrievedFromCache(CacheStorageCacheHandle cache_handle,
                              base::OnceClosure closure,
                              int64_t* accumulator,
                              int64_t size);

  void NotifyCacheContentChanged(const std::string& cache_name);

  void ScheduleWriteIndex();
  void WriteIndex(base::OnceCallback<void(bool)> callback);
  void WriteIndexImpl(base::OnceCallback<void(bool)> callback);
  bool index_write_pending() const { return !index_write_task_.IsCancelled(); }
  // Start a scheduled index write immediately. Returns true if a write was
  // scheduled, or false if not.
  bool InitiateScheduledIndexWriteForTest(
      base::OnceCallback<void(bool)> callback);

  // Whether or not we've loaded the list of cache names into memory.
  bool initialized_;
  bool initializing_;

  // True if the backend is supposed to reside in memory only.
  bool memory_only_;

  // The pending operation scheduler.
  std::unique_ptr<CacheStorageScheduler> scheduler_;

  // The map of cache names to CacheStorageCache objects.
  CacheMap cache_map_;

  // Caches that have been deleted but must still be held onto until all handles
  // have been released.
  std::map<CacheStorageCache*, std::unique_ptr<CacheStorageCache>>
      doomed_caches_;

  // CacheStorageCacheHandle reference counts
  std::map<CacheStorageCache*, size_t> cache_handle_counts_;

  // The cache index data.
  std::unique_ptr<CacheStorageIndex> cache_index_;

  // The file path for this CacheStorage.
  base::FilePath origin_path_;

  // The TaskRunner to run file IO on.
  scoped_refptr<base::SequencedTaskRunner> cache_task_runner_;

  // Performs backend specific operations (memory vs disk).
  std::unique_ptr<CacheLoader> cache_loader_;

  // The quota manager.
  scoped_refptr<storage::QuotaManagerProxy> quota_manager_proxy_;

  // The origin that this CacheStorage is associated with.
  url::Origin origin_;

  // The owner that this CacheStorage is associated with.
  CacheStorageOwner owner_;

  // The manager that owns this cache storage. Only set to null by
  // RemoveManager() when this cache storage is being deleted.
  CacheStorageManager* cache_storage_manager_;

  base::CancelableOnceClosure index_write_task_;

  base::WeakPtrFactory<CacheStorage> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CacheStorage);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_H_
