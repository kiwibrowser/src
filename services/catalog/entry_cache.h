// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_CATALOG_ENTRY_CACHE_H_
#define SERVICES_CATALOG_ENTRY_CACHE_H_

#include <map>
#include <memory>
#include <string>

#include "base/component_export.h"
#include "base/macros.h"

namespace catalog {

class Entry;

// Indexed storage for all existing service catalog entries.
class COMPONENT_EXPORT(CATALOG) EntryCache {
 public:
  EntryCache();
  ~EntryCache();

  // All entries in the cache, including non-root entries.
  const std::map<std::string, const Entry*>& entries() const {
    return entries_;
  }

  // Adds a new root entry to the cache. If a root entry already exists
  // corresponding to the new entry's name, the old root entry is removed first,
  // along with its children.
  //
  // If a non-root entry already exists corresponding to the new entry's name,
  // the new entry is ignored.
  //
  // Returns |true| if the entry was added and |false| otherwise.
  //
  // TODO(rockot): Duplicate entries should be treated as an error, but for now
  // we tolerate them because of some remaining dependency on Package directory
  // scanning, which in turn has some unpredictable behavior with respect to
  // Entry registration.
  bool AddRootEntry(std::unique_ptr<Entry> entry);

  // Queries the cache for an entry corresponding to |name|. Returns null if
  // such an entry is not found.
  const Entry* GetEntry(const std::string& name);

 private:
  // Adds and entry and its children to |entries_|. Returns |true| if the Entry
  // was successfully added and |false| otherwise.
  bool AddEntry(const Entry* entry);

  // Removes an entry and its children from |entries_|.
  void RemoveEntry(const Entry* entry);

  // Map of top-level service entries. This transitively owns all existing
  // Entry objects.
  std::map<std::string, std::unique_ptr<Entry>> root_entries_;

  // Map of service name to Entry. Each value points to an Entry owned either
  // directly or indirectly by |entries_| above. This is essentially a flattened
  // version of |entries_| with no ownership.
  std::map<std::string, const Entry*> entries_;

  DISALLOW_COPY_AND_ASSIGN(EntryCache);
};

}  // namespace catalog

#endif  // SERVICES_CATALOG_ENTRY_CACHE_H_
