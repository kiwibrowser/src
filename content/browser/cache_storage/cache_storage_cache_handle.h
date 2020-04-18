// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_CACHE_HANDLE_H_
#define CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_CACHE_HANDLE_H_

#include "base/memory/weak_ptr.h"
#include "content/browser/cache_storage/cache_storage.h"
#include "content/browser/cache_storage/cache_storage_cache.h"

namespace content {

// Holds a reference to a CacheStorageCache. The CacheStorage will keep the
// CacheStorageCache alive until the last handle is destroyed or the
// CacheStorage is deleted.
class CONTENT_EXPORT CacheStorageCacheHandle {
 public:
  CacheStorageCacheHandle();
  CacheStorageCacheHandle(CacheStorageCacheHandle&& other);
  ~CacheStorageCacheHandle();

  // Returns the underlying CacheStorageCache*. Will return nullptr if the
  // CacheStorage has been deleted.
  CacheStorageCache* value() { return cache_storage_cache_.get(); }

  CacheStorageCacheHandle Clone() const;

  CacheStorageCacheHandle& operator=(CacheStorageCacheHandle&& rhs);

 private:
  friend class CacheStorage;

  CacheStorageCacheHandle(base::WeakPtr<CacheStorageCache> cache_storage_cache,
                          base::WeakPtr<CacheStorage> cache_storage);

  base::WeakPtr<CacheStorageCache> cache_storage_cache_;
  base::WeakPtr<CacheStorage> cache_storage_;

  DISALLOW_COPY_AND_ASSIGN(CacheStorageCacheHandle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_CACHE_HANDLE_H_
