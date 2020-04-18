// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_MODEL_NEUTRAL_MUTABLE_ENTRY_H_
#define COMPONENTS_SYNC_SYNCABLE_MODEL_NEUTRAL_MUTABLE_ENTRY_H_

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "base/macros.h"
#include "components/sync/base/model_type.h"
#include "components/sync/syncable/entry.h"

namespace syncer {

class WriteNode;

namespace syncable {

class BaseWriteTransaction;

enum CreateNewUpdateItem { CREATE_NEW_UPDATE_ITEM };

enum CreateNewTypeRoot { CREATE_NEW_TYPE_ROOT };

// This Entry includes all the operations one can safely perform on the sync
// thread.  In particular, it does not expose setters to make changes that need
// to be communicated to the model (and the model's thread).  It is not possible
// to change an entry's SPECIFICS or UNIQUE_POSITION fields with this kind of
// entry.
class ModelNeutralMutableEntry : public Entry {
 public:
  ModelNeutralMutableEntry(BaseWriteTransaction* trans,
                           CreateNewUpdateItem,
                           const Id& id);
  ModelNeutralMutableEntry(BaseWriteTransaction* trans,
                           CreateNewTypeRoot,
                           ModelType type);
  ModelNeutralMutableEntry(BaseWriteTransaction* trans, GetByHandle, int64_t);
  ModelNeutralMutableEntry(BaseWriteTransaction* trans, GetById, const Id&);
  ModelNeutralMutableEntry(BaseWriteTransaction* trans,
                           GetByClientTag,
                           const std::string& tag);
  ModelNeutralMutableEntry(BaseWriteTransaction* trans,
                           GetTypeRoot,
                           ModelType type);

  inline BaseWriteTransaction* base_write_transaction() const {
    return base_write_transaction_;
  }

  // Non-model-changing setters.  These setters will change properties internal
  // to the node.  These fields are important for bookkeeping in the sync
  // internals, but it is not necessary to communicate changes in these fields
  // to the local models.
  //
  // Some of them trigger the re-indexing of the entry.  They return true on
  // success and false on failure, which occurs when putting the value would
  // have caused a duplicate in the index.  The setters that never fail return
  // void.
  void PutBaseVersion(int64_t value);
  void PutServerVersion(int64_t value);
  void PutServerMtime(base::Time value);
  void PutServerCtime(base::Time value);
  bool PutId(const Id& value);
  void PutServerParentId(const Id& value);
  bool PutIsUnsynced(bool value);
  bool PutIsUnappliedUpdate(bool value);
  void PutServerIsDir(bool value);
  void PutServerIsDel(bool value);
  void PutServerNonUniqueName(const std::string& value);
  bool PutUniqueServerTag(const std::string& value);
  bool PutUniqueClientTag(const std::string& value);
  void PutUniqueBookmarkTag(const std::string& tag);
  void PutServerSpecifics(const sync_pb::EntitySpecifics& value);
  void PutBaseServerSpecifics(const sync_pb::EntitySpecifics& value);
  void PutServerUniquePosition(const UniquePosition& value);
  void PutSyncing(bool value);
  void PutDirtySync(bool value);

  // Do a simple property-only update of the PARENT_ID field.  Use with caution.
  //
  // The normal Put(IS_PARENT) call will move the item to the front of the
  // sibling order to maintain the linked list invariants when the parent
  // changes.  That's usually what you want to do, but it's inappropriate
  // when the caller is trying to change the parent ID of a the whole set
  // of children (e.g. because the ID changed during a commit).  For those
  // cases, there's this function.  It will corrupt the sibling ordering
  // if you're not careful.
  void PutParentIdPropertyOnly(const Id& parent_id);

  // This is similar to what one would expect from Put(TRANSACTION_VERSION),
  // except that it doesn't bother to invoke 'SaveOriginals'.  Calling that
  // function is at best unnecessary, since the transaction will have already
  // used its list of mutations by the time this function is called.
  void UpdateTransactionVersion(int64_t version);

 protected:
  explicit ModelNeutralMutableEntry(BaseWriteTransaction* trans);

  void MarkDirty();

 private:
  friend class syncer::WriteNode;
  friend class syncer::syncable::Directory;

  // Don't allow creation on heap, except by sync API wrappers.
  void* operator new(size_t size) { return (::operator new)(size); }

  // Kind of redundant. We should reduce the number of pointers
  // floating around if at all possible. Could we store this in Directory?
  // Scope: Set on construction, never changed after that.
  BaseWriteTransaction* const base_write_transaction_;

  DISALLOW_COPY_AND_ASSIGN(ModelNeutralMutableEntry);
};

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_MODEL_NEUTRAL_MUTABLE_ENTRY_H_
