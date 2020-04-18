// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SYNCABLE_SYNCABLE_UTIL_H_
#define COMPONENTS_SYNC_SYNCABLE_SYNCABLE_UTIL_H_

#include <stdint.h>

#include <vector>

#include "components/sync/syncable/directory.h"

namespace base {
class Location;
}

namespace syncer {
namespace syncable {

class BaseTransaction;
class BaseWriteTransaction;
class ModelNeutralMutableEntry;
class Id;

void ChangeEntryIDAndUpdateChildren(BaseWriteTransaction* trans,
                                    ModelNeutralMutableEntry* entry,
                                    const Id& new_id);

bool IsLegalNewParent(BaseTransaction* trans, const Id& id, const Id& parentid);

bool SyncAssert(bool condition,
                const base::Location& location,
                const char* msg,
                BaseTransaction* trans);

int GetUnsyncedEntries(BaseTransaction* trans, Directory::Metahandles* handles);

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_SYNCABLE_SYNCABLE_UTIL_H_
