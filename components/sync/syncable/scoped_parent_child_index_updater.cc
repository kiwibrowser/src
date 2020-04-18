// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/syncable/scoped_parent_child_index_updater.h"

#include "components/sync/syncable/parent_child_index.h"

namespace syncer {
namespace syncable {

ScopedParentChildIndexUpdater::ScopedParentChildIndexUpdater(
    const ScopedKernelLock& proof_of_lock,
    EntryKernel* entry,
    ParentChildIndex* index)
    : entry_(entry), index_(index) {
  if (ParentChildIndex::ShouldInclude(entry_)) {
    index_->Remove(entry_);
  }
}

ScopedParentChildIndexUpdater::~ScopedParentChildIndexUpdater() {
  if (ParentChildIndex::ShouldInclude(entry_)) {
    index_->Insert(entry_);
  }
}

}  // namespace syncable
}  // namespace syncer
