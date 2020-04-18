// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_MUTABLE_ENTRY_H_
#define COMPONENTS_SYNC_SYNCABLE_MUTABLE_ENTRY_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/sync/base/model_type.h"
#include "components/sync/syncable/entry.h"
#include "components/sync/syncable/metahandle_set.h"
#include "components/sync/syncable/model_neutral_mutable_entry.h"

namespace syncer {

namespace syncable {

enum Create { CREATE };

class WriteTransaction;

// A mutable meta entry.  Changes get committed to the database when the
// WriteTransaction is destroyed.
class MutableEntry : public ModelNeutralMutableEntry {
 public:
  MutableEntry(WriteTransaction* trans, CreateNewUpdateItem, const Id& id);
  MutableEntry(WriteTransaction* trans,
               Create,
               ModelType model_type,
               const std::string& name);
  MutableEntry(WriteTransaction* trans,
               Create,
               ModelType model_type,
               const Id& parent_id,
               const std::string& name);
  MutableEntry(WriteTransaction* trans, GetByHandle, int64_t);
  MutableEntry(WriteTransaction* trans, GetById, const Id&);
  MutableEntry(WriteTransaction* trans, GetByClientTag, const std::string& tag);
  MutableEntry(WriteTransaction* trans, GetTypeRoot, ModelType type);

  inline WriteTransaction* write_transaction() const {
    return write_transaction_;
  }

  // Model-changing setters.  These setters make user-visible changes that will
  // need to be communicated either to the local model or the sync server.
  void PutLocalExternalId(int64_t value);
  void PutMtime(base::Time value);
  void PutCtime(base::Time value);
  void PutParentId(const Id& value);
  void PutIsDir(bool value);
  void PutIsDel(bool value);
  void PutNonUniqueName(const std::string& value);
  void PutSpecifics(const sync_pb::EntitySpecifics& value);
  void PutUniquePosition(const UniquePosition& value);

  // Sets the position of this item, and updates the entry kernels of the
  // adjacent siblings so that list invariants are maintained.  Returns false
  // and fails if |predecessor_id| does not identify a sibling.  Pass the root
  // ID to put the node in first position.
  bool PutPredecessor(const Id& predecessor_id);

 private:
  static std::unique_ptr<EntryKernel> CreateEntryKernel(
      WriteTransaction* trans,
      ModelType model_type,
      const Id& parent_id,
      const std::string& name);

  // Kind of redundant. We should reduce the number of pointers
  // floating around if at all possible. Could we store this in Directory?
  // Scope: Set on construction, never changed after that.
  WriteTransaction* const write_transaction_;

  DISALLOW_COPY_AND_ASSIGN(MutableEntry);
};

// This function sets only the flags needed to get this entry to sync.
bool MarkForSyncing(syncable::MutableEntry* e);

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_MUTABLE_ENTRY_H_
