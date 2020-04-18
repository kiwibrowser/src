// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOM_DISTILLER_CORE_DOM_DISTILLER_MODEL_H_
#define COMPONENTS_DOM_DISTILLER_CORE_DOM_DISTILLER_MODEL_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/containers/id_map.h"
#include "base/macros.h"
#include "components/dom_distiller/core/article_entry.h"
#include "components/sync/model/sync_change.h"
#include "components/sync/model/sync_change_processor.h"  // syncer::SyncChangeList
#include "components/sync/model/sync_data.h"
#include "url/gurl.h"

namespace dom_distiller {

// This stores the in-memory model of the DOM distiller list. Entries can be
// looked up by URL or by entry_id.
// The model assumes that an URL corresponds to at most a single entry. If this
// assumption is broken, lookup by URL may return unexpected results.
class DomDistillerModel {
 public:
  DomDistillerModel();
  explicit DomDistillerModel(const std::vector<ArticleEntry>& initial_data);

  ~DomDistillerModel();

  // Lookup an ArticleEntry by ID or URL. Returns whether a corresponding entry
  // was found. On success, if |entry| is not null, it will contain the entry.
  bool GetEntryById(const std::string& entry_id, ArticleEntry* entry) const;
  bool GetEntryByUrl(const GURL& url, ArticleEntry* entry) const;

  std::vector<ArticleEntry> GetEntries() const;
  size_t GetNumEntries() const;

  syncer::SyncDataList GetAllSyncData() const;

  // Convert a SyncDataList to a SyncChangeList of add or update changes based
  // on the state of the model. Also calculate the entries missing from the
  // SyncDataList.
  void CalculateChangesForMerge(const syncer::SyncDataList& data,
                                syncer::SyncChangeList* changes_to_apply,
                                syncer::SyncChangeList* changes_missing);

  // Applies the change list to the model, appending the actual changes made to
  // the model to |changes_applied|. If conflict resolution does not apply the
  // requested change, then adds the "diff" between what was requested and what
  // was actually applied to |changes_missing|.
  // Note: Currently conflicts are resolved by just applying the requested
  // change. This means nothing will be added to |changes_missing|.
  void ApplyChangesToModel(const syncer::SyncChangeList& change_list,
                           syncer::SyncChangeList* changes_applied,
                           syncer::SyncChangeList* changes_missing);

 private:
  typedef int32_t KeyType;
  typedef base::hash_map<KeyType, ArticleEntry> EntryMap;
  typedef base::hash_map<std::string, KeyType> StringToKeyMap;

  void AddEntry(const ArticleEntry& entry);
  void RemoveEntry(const ArticleEntry& entry);

  // Lookup an entry's key by ID or URL. Returns whether a corresponding key was
  // found. On success, if |key| is not null, it will contain the entry.
  bool GetKeyById(const std::string& entry_id, KeyType* key) const;
  bool GetKeyByUrl(const GURL& url, KeyType* key) const;

  // If |entry| is not null, assigns the entry for |key| to it. |key| must map
  // to an entry in |entries_|.
  void GetEntryByKey(KeyType key, ArticleEntry* entry) const;

  void ApplyChangeToModel(const syncer::SyncChange& change,
                          syncer::SyncChangeList* changes_applied,
                          syncer::SyncChangeList* changes_missing);

  KeyType next_key_;
  EntryMap entries_;
  StringToKeyMap url_to_key_map_;
  StringToKeyMap entry_id_to_key_map_;

  DISALLOW_COPY_AND_ASSIGN(DomDistillerModel);
};

}  // namespace dom_distiller

#endif  // COMPONENTS_DOM_DISTILLER_CORE_DOM_DISTILLER_MODEL_H_
