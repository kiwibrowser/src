// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_disk_cache.h"

namespace content {

ServiceWorkerDiskCache::ServiceWorkerDiskCache()
    : AppCacheDiskCache(true /* use_simple_cache */) {
  uma_name_ = "DiskCache.ServiceWorker";
}

ServiceWorkerResponseReader::ServiceWorkerResponseReader(
    int64_t resource_id,
    const base::WeakPtr<AppCacheDiskCacheInterface>& disk_cache)
    : AppCacheResponseReader(resource_id, disk_cache) {}

ServiceWorkerResponseWriter::ServiceWorkerResponseWriter(
    int64_t resource_id,
    const base::WeakPtr<AppCacheDiskCacheInterface>& disk_cache)
    : AppCacheResponseWriter(resource_id, disk_cache) {}

ServiceWorkerResponseMetadataWriter::ServiceWorkerResponseMetadataWriter(
    int64_t resource_id,
    const base::WeakPtr<AppCacheDiskCacheInterface>& disk_cache)
    : AppCacheResponseMetadataWriter(resource_id, disk_cache) {}

}  // namespace content
