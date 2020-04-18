// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/conflict_util.h"

#include "components/sync/syncable/mutable_entry.h"

namespace syncer {

using syncable::MutableEntry;

namespace conflict_util {

// Allow the server's changes to take precedence.
// This will take effect during the next ApplyUpdates step.
void IgnoreLocalChanges(MutableEntry* entry) {
  DCHECK(entry->GetIsUnsynced());
  DCHECK(entry->GetIsUnappliedUpdate());
  entry->PutIsUnsynced(false);
}

// Overwrite the server with our own value.
// We will commit our local data, overwriting the server, at the next
// opportunity.
void OverwriteServerChanges(MutableEntry* entry) {
  DCHECK(entry->GetIsUnsynced());
  DCHECK(entry->GetIsUnappliedUpdate());
  entry->PutBaseVersion(entry->GetServerVersion());
  entry->PutIsUnappliedUpdate(false);
}

// Having determined that everything matches, we ignore the non-conflict.
void IgnoreConflict(MutableEntry* entry) {
  // If we didn't also unset IS_UNAPPLIED_UPDATE, then we would lose unsynced
  // positional data from adjacent entries when the server update gets applied
  // and the item is re-inserted into the PREV_ID/NEXT_ID linked list. This is
  // primarily an issue because we commit after applying updates, and is most
  // commonly seen when positional changes are made while a passphrase is
  // required (and hence there will be many encryption conflicts).
  DCHECK(entry->GetIsUnsynced());
  DCHECK(entry->GetIsUnappliedUpdate());
  entry->PutBaseVersion(entry->GetServerVersion());
  entry->PutIsUnappliedUpdate(false);
  entry->PutIsUnsynced(false);
}

}  // namespace conflict_util
}  // namespace syncer
