// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_CHANGE_REORDER_BUFFER_H_
#define COMPONENTS_SYNC_SYNCABLE_CHANGE_REORDER_BUFFER_H_

#include <stdint.h>

#include <map>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/syncable/change_record.h"

namespace syncer {

class BaseTransaction;

// ChangeReorderBuffer is a utility type which accepts an unordered set
// of changes (via its Push methods), and yields an ImmutableChangeRecordList
// (via the GetAllChangesInTreeOrder method) that are in the order that
// the SyncObserver expects them to be. A buffer is initially empty.
//
// The ordering produced by ChangeReorderBuffer is as follows:
//  (a) All Deleted items appear first.
//  (b) For Updated and/or Added items, parents appear before their children.
//
// The sibling order is not necessarily preserved.
class ChangeReorderBuffer {
 public:
  ChangeReorderBuffer();
  ~ChangeReorderBuffer();

  // Insert an item, identified by the metahandle |id|, into the reorder buffer.
  // This item will appear in the output list as an ACTION_ADD ChangeRecord.
  void PushAddedItem(int64_t id);

  // Insert an item, identified by the metahandle |id|, into the reorder buffer.
  // This item will appear in the output list as an ACTION_DELETE ChangeRecord.
  void PushDeletedItem(int64_t id);

  // Insert an item, identified by the metahandle |id|, into the reorder buffer.
  // This item will appear in the output list as an ACTION_UPDATE ChangeRecord.
  void PushUpdatedItem(int64_t id);

  void SetExtraDataForId(int64_t id, ExtraPasswordChangeRecordData* extra);

  void SetSpecificsForId(int64_t id, const sync_pb::EntitySpecifics& specifics);

  // Reset the buffer, forgetting any pushed items, so that it can be used again
  // to reorder a new set of changes.
  void Clear();

  bool IsEmpty() const;

  // Output a reordered list of changes to |changes| using the items
  // that were pushed into the reorder buffer. |sync_trans| is used to
  // determine the ordering.  Returns true if successful, or false if
  // an error was encountered.
  bool GetAllChangesInTreeOrder(const BaseTransaction* sync_trans,
                                ImmutableChangeRecordList* changes)
      WARN_UNUSED_RESULT;

 private:
  class Traversal;
  using OperationMap = std::map<int64_t, ChangeRecord::Action>;
  using SpecificsMap = std::map<int64_t, sync_pb::EntitySpecifics>;
  using ExtraDataMap =
      std::map<int64_t, linked_ptr<ExtraPasswordChangeRecordData>>;

  // Stores the items that have been pushed into the buffer, and the type of
  // operation that was associated with them.
  OperationMap operations_;

  // Stores entity-specific ChangeRecord data per-ID.
  SpecificsMap specifics_;

  // Stores type-specific extra data per-ID.
  ExtraDataMap extra_data_;

  DISALLOW_COPY_AND_ASSIGN(ChangeReorderBuffer);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_CHANGE_REORDER_BUFFER_H_
