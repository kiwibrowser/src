// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_DOM_STORAGE_LOCAL_STORAGE_CACHED_AREAS_H_
#define CONTENT_RENDERER_DOM_STORAGE_LOCAL_STORAGE_CACHED_AREAS_H_

#include <map>
#include <string>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/common/storage_partition_service.mojom.h"
#include "url/origin.h"

namespace blink {
namespace scheduler {
class WebMainThreadScheduler;
}
}  // namespace blink

namespace content {
class LocalStorageCachedArea;

namespace mojom {
class StoragePartitionService;
}

// Keeps a map of all the LocalStorageCachedArea objects in a renderer. This is
// needed because we can have n LocalStorageArea objects for the same origin but
// we want just one LocalStorageCachedArea to service them (no point in having
// multiple caches of the same data in the same process).
// TODO(dmurph): Rename to remove LocalStorage.
class CONTENT_EXPORT LocalStorageCachedAreas {
 public:
  LocalStorageCachedAreas(
      mojom::StoragePartitionService* storage_partition_service,
      blink::scheduler::WebMainThreadScheduler* main_thread_scheduler);
  ~LocalStorageCachedAreas();

  // Returns, creating if necessary, a cached storage area for the given origin.
  scoped_refptr<LocalStorageCachedArea>
      GetCachedArea(const url::Origin& origin);

  scoped_refptr<LocalStorageCachedArea> GetSessionStorageArea(
      const std::string& namespace_id,
      const url::Origin& origin);

  void CloneNamespace(const std::string& source_namespace,
                      const std::string& destination_namespace);

  size_t TotalCacheSize() const;

  void set_cache_limit_for_testing(size_t limit) { total_cache_limit_ = limit; }

 private:
  void ClearAreasIfNeeded();

  scoped_refptr<LocalStorageCachedArea> GetCachedArea(
      const std::string& namespace_id,
      const url::Origin& origin,
      blink::scheduler::WebMainThreadScheduler* scheduler);

  mojom::StoragePartitionService* const storage_partition_service_;

  struct DOMStorageNamespace {
   public:
    DOMStorageNamespace();
    ~DOMStorageNamespace();
    DOMStorageNamespace(DOMStorageNamespace&& other);
    DOMStorageNamespace& operator=(DOMStorageNamespace&&) = default;

    size_t TotalCacheSize() const;
    // Returns true if this namespace is totally unused and can be deleted.
    bool CleanUpUnusedAreas();

    mojom::SessionStorageNamespacePtr session_storage_namespace;
    base::flat_map<url::Origin, scoped_refptr<LocalStorageCachedArea>>
        cached_areas;

    DISALLOW_COPY_AND_ASSIGN(DOMStorageNamespace);
  };

  base::flat_map<std::string, DOMStorageNamespace> cached_namespaces_;
  size_t total_cache_limit_;

  // Not owned.
  blink::scheduler::WebMainThreadScheduler* main_thread_scheduler_;

  DISALLOW_COPY_AND_ASSIGN(LocalStorageCachedAreas);
};

}  // namespace content

#endif  // CONTENT_RENDERER_DOM_STORAGE_LOCAL_STORAGE_CACHED_AREAS_H_
