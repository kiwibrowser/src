// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_DELETE_JOURNAL_H_
#define COMPONENTS_SYNC_SYNCABLE_DELETE_JOURNAL_H_

#include <stdint.h>

#include <set>
#include <vector>

#include "components/sync/base/model_type.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {

class BaseTransaction;

struct BookmarkDeleteJournal {
  int64_t id;           // Metahandle of delete journal entry.
  int64_t external_id;  // Bookmark ID in the native model.
  bool is_folder;
  sync_pb::EntitySpecifics specifics;
};
using BookmarkDeleteJournalList = std::vector<BookmarkDeleteJournal>;

// Static APIs for passing delete journals between syncable namspace
// and syncer namespace.
class DeleteJournal {
 public:
  // Return info about deleted bookmark entries stored in the delete journal
  // of |trans|'s directory.
  // TODO(haitaol): remove this after SyncData supports bookmarks and
  //                all types of delete journals can be returned as
  //                SyncDataList.
  static void GetBookmarkDeleteJournals(
      BaseTransaction* trans,
      BookmarkDeleteJournalList* delete_journals);

  // Purge delete journals of given IDs from |trans|'s directory.
  static void PurgeDeleteJournals(BaseTransaction* trans,
                                  const std::set<int64_t>& ids);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_DELETE_JOURNAL_H_
