// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_NAMESPACE_H_
#define CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_NAMESPACE_H_

#include <stdint.h>

#include <map>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace base {
namespace trace_event {
class ProcessMemoryDump;
}
}

namespace url {
class Origin;
}

namespace content {

class DOMStorageArea;
class DOMStorageTaskRunner;
class SessionStorageDatabase;

// Container for the set of per-origin Areas.
// See class comments for DOMStorageContextImpl for a larger overview.
class CONTENT_EXPORT DOMStorageNamespace
    : public base::RefCountedThreadSafe<DOMStorageNamespace> {
 public:
  // Struct to hold statistics about the areas in the namespace.
  struct UsageStatistics {
    size_t total_cache_size;
    unsigned total_area_count;
    unsigned inactive_area_count;  // areas with open count 0.
  };

  // Constructor for a SessionStorage namespace with the given namespace id and
  // an optional backing on disk via |session_storage_database| (may be NULL).
  DOMStorageNamespace(const std::string& namespace_id,
                      SessionStorageDatabase* session_storage_database,
                      DOMStorageTaskRunner* task_runner);

  const std::string& namespace_id() const { return namespace_id_; }

  // Returns the storage area for the given origin,
  // creating instance if needed. Each call to open
  // must be balanced with a call to CloseStorageArea.
  DOMStorageArea* OpenStorageArea(const url::Origin& origin);
  void CloseStorageArea(DOMStorageArea* area);

  // Returns the area for |origin| if it's open, otherwise NULL.
  DOMStorageArea* GetOpenStorageArea(const url::Origin& origin);

  // Creates a clone of |this| namespace including
  // shallow copies of all contained areas.
  // Should only be called for session storage namespaces.
  DOMStorageNamespace* Clone(const std::string& clone_namespace_id);

  void DeleteSessionStorageOrigin(const url::Origin& origin);
  void PurgeMemory(bool aggressively);
  void Shutdown();
  void Flush();

  // Returns statistics about the areas in the namespace.
  UsageStatistics GetUsageStatistics() const;

  // Adds memory statistics to |pmd| for chrome://tracing.
  void OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd);

  void GetOriginsWithAreas(std::vector<url::Origin>* origins) const;

  int GetAreaOpenCount(const url::Origin& origin) const;

 private:
  friend class base::RefCountedThreadSafe<DOMStorageNamespace>;

  // Struct to hold references to our contained areas and
  // to keep track of how many tabs have a given area open.
  struct AreaHolder {
    scoped_refptr<DOMStorageArea> area_;
    int open_count_;

    AreaHolder();
    AreaHolder(DOMStorageArea* area, int count);
    AreaHolder& operator=(AreaHolder&& other);
    ~AreaHolder();
    DISALLOW_COPY_AND_ASSIGN(AreaHolder);
  };
  typedef std::map<url::Origin, AreaHolder> AreaMap;

  ~DOMStorageNamespace();

  // Returns a pointer to the area holder in our map or NULL.
  AreaHolder* GetAreaHolder(const url::Origin& origin);

  void OnCloneStorageDone();

  const std::string namespace_id_;
  AreaMap areas_;
  const scoped_refptr<DOMStorageTaskRunner> task_runner_;
  const scoped_refptr<SessionStorageDatabase> session_storage_database_;
};

}  // namespace content


#endif  // CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_NAMESPACE_H_
