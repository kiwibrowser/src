// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_DELETE_JOURNAL_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_DELETE_JOURNAL_H_

#include <stddef.h>

#include <map>
#include <memory>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "components/sync/syncable/entry_kernel.h"
#include "components/sync/syncable/metahandle_set.h"

namespace syncer {
namespace syncable {

class BaseTransaction;

class LessByID {
 public:
  inline bool operator()(const EntryKernel* a, const EntryKernel* b) const {
    return a->ref(ID) < b->ref(ID);
  }
};
// This should be |std::set<std::unique_ptr<EntryKernel>, LessByID>|. However,
// DeleteJournal::TakeSnapshotAndClear needs to remove the unique_ptr from the
// set, and std::set::extract is only available in C++17. TODO(avi): Switch to
// using a std::set when Chromium allows C++17 use.
using JournalIndex =
    std::map<const EntryKernel*, std::unique_ptr<EntryKernel>, LessByID>;

// DeleteJournal manages deleted entries that are not in sync directory until
// it's safe to drop them after the deletion is confirmed with native models.
// DeleteJournal is thread-safe and can be accessed on any thread. Has to hold
// a valid transaction object when calling methods of DeleteJournal, thus each
// method requires a non-null |trans| parameter.
class DeleteJournal {
 public:
  // Initialize |delete_journals_| using |initial_journal|.
  explicit DeleteJournal(std::unique_ptr<JournalIndex> initial_journal);
  ~DeleteJournal();

  // For testing only.
  size_t GetDeleteJournalSize(BaseTransaction* trans) const;

  // Add/remove |entry| to/from |delete_journals_| according to its
  // SERVER_IS_DEL field and |was_deleted|. Called on sync thread.
  void UpdateDeleteJournalForServerDelete(BaseTransaction* trans,
                                          bool was_deleted,
                                          const EntryKernel& entry);

  // Return entries of specified type in |delete_journals_|. This should be
  // called ONCE in model association. |deleted_entries| can be used to
  // detect deleted sync data that's not persisted in native model to
  // prevent back-from-dead problem. |deleted_entries| are only valid during
  // lifetime of |trans|. |type| is added to |passive_delete_journal_types_| to
  // enable periodically saving/clearing of delete journals of |type| because
  // new journals added later are not needed until next model association.
  // Can be called on any thread.
  void GetDeleteJournals(BaseTransaction* trans,
                         ModelType type,
                         EntryKernelSet* deleted_entries);

  // Purge entries of specified type in |delete_journals_| if their handles are
  // in |to_purge|. This should be called after model association and
  // |to_purge| should contain handles of the entries whose deletions are
  // confirmed in native model. Can be called on any thread.
  void PurgeDeleteJournals(BaseTransaction* trans,
                           const MetahandleSet& to_purge);

  // Move entries in |delete_journals_| whose types are in
  // |passive_delete_journal_types_| to |journal_entries|. Move handles in
  // |delete_journals_to_purge_| to |journals_to_purge|. Called on sync thread.
  void TakeSnapshotAndClear(BaseTransaction* trans,
                            OwnedEntryKernelSet* journal_entries,
                            MetahandleSet* journals_to_purge);

  // Add |entries| to |delete_journals_| regardless of their SERVER_IS_DEL
  // value. This is used to:
  // * restore delete journals from snapshot if snapshot failed to save.
  // * batch add entries of a data type with unrecoverable error to delete
  //   journal before purging them.
  // Called on sync thread.
  void AddJournalBatch(BaseTransaction* trans,
                       const OwnedEntryKernelSet& entries);

  // Return true if delete journals of |type| are maintained.
  static bool IsDeleteJournalEnabled(ModelType type);

  // Adds entry to JournalIndex if it doesn't already exist.
  static void AddEntryToJournalIndex(JournalIndex* journal_index,
                                     std::unique_ptr<EntryKernel> entry);

 private:
  FRIEND_TEST_ALL_PREFIXES(SyncableDirectoryTest, ManageDeleteJournals);

  // Contains deleted entries that may not be persisted in native models. And
  // in case of unrecoverable error, all purged entries are moved here for
  // bookkeeping to prevent back-from-dead entries that are deleted elsewhere
  // when sync's down.
  JournalIndex delete_journals_;

  // Contains meta handles of deleted entries that have been persisted or
  // undeleted, thus can be removed from database.
  MetahandleSet delete_journals_to_purge_;

  // Delete journals of these types can be cleared from memory after being
  // saved to database.
  ModelTypeSet passive_delete_journal_types_;

  DISALLOW_COPY_AND_ASSIGN(DeleteJournal);
};

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_DELETE_JOURNAL_H_
