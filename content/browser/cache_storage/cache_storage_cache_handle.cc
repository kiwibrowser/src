// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "content/browser/cache_storage/cache_storage_cache_handle.h"

namespace content {

CacheStorageCacheHandle::CacheStorageCacheHandle() = default;

CacheStorageCacheHandle::CacheStorageCacheHandle(
    CacheStorageCacheHandle&& other)
    : cache_storage_cache_(std::move(other.cache_storage_cache_)),
      cache_storage_(std::move(other.cache_storage_)) {}

CacheStorageCacheHandle::~CacheStorageCacheHandle() {
  if (cache_storage_ && cache_storage_cache_)
    cache_storage_->DropCacheHandleRef(cache_storage_cache_.get());
}

CacheStorageCacheHandle CacheStorageCacheHandle::Clone() const {
  return CacheStorageCacheHandle(cache_storage_cache_, cache_storage_);
}

CacheStorageCacheHandle::CacheStorageCacheHandle(
    base::WeakPtr<CacheStorageCache> cache_storage_cache,
    base::WeakPtr<CacheStorage> cache_storage)
    : cache_storage_cache_(cache_storage_cache), cache_storage_(cache_storage) {
  DCHECK(cache_storage);
  DCHECK(cache_storage_cache_);
  cache_storage_->AddCacheHandleRef(cache_storage_cache_.get());
}

CacheStorageCacheHandle& CacheStorageCacheHandle::operator=(
    CacheStorageCacheHandle&& rhs) {
  if (cache_storage_ && cache_storage_cache_)
    cache_storage_->DropCacheHandleRef(cache_storage_cache_.get());
  cache_storage_cache_ = std::move(rhs.cache_storage_cache_);
  cache_storage_ = std::move(rhs.cache_storage_);
  return *this;
}

}  // namespace content
