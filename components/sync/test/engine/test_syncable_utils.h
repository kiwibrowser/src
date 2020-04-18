// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_ENGINE_TEST_SYNCABLE_UTILS_H_
#define COMPONENTS_SYNC_TEST_ENGINE_TEST_SYNCABLE_UTILS_H_

#include <string>

#include "components/sync/base/model_type.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {
namespace syncable {

class BaseTransaction;
class Directory;
class Id;
class WriteTransaction;

// Utilities that are useful in verifying the state of items in a
// syncable database.

// Count the number of entries with a given name inside of a parent.
// Useful to check folder structure and for porting older tests that
// rely on uniqueness inside of folders.
int CountEntriesWithName(BaseTransaction* rtrans,
                         const syncable::Id& parent_id,
                         const std::string& name);

// Get the first entry ID with name in a parent. The entry *must* exist.
Id GetFirstEntryWithName(BaseTransaction* rtrans,
                         const syncable::Id& parent_id,
                         const std::string& name);

// Assert that there's only one entry by this name in this parent.
// Return the Id.
Id GetOnlyEntryWithName(BaseTransaction* rtrans,
                        const syncable::Id& parent_id,
                        const std::string& name);

void CreateTypeRoot(WriteTransaction* trans,
                    syncable::Directory* dir,
                    ModelType type);

sync_pb::DataTypeProgressMarker BuildProgress(ModelType type);

}  // namespace syncable
}  // namespace syncer

#endif  // COMPONENTS_SYNC_TEST_ENGINE_TEST_SYNCABLE_UTILS_H_
