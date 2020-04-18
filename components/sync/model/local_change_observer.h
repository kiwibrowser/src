// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_MODEL_LOCAL_CHANGE_OBSERVER_H_
#define COMPONENTS_SYNC_MODEL_LOCAL_CHANGE_OBSERVER_H_

namespace syncer {

class SyncChange;
namespace syncable {
class Entry;
}  // namespace syncable

// Interface for observers that want to be notified of local sync changes.
// The OnLocalChange function will be called when a local change made through
// ProcessSyncChanges is about to be applied to the directory. This is useful
// for inspecting the specifics of sync data being written locally and comparing
// it against the current state of the directory. Registering a local change
// observer is done by calling the AddLocalChangeObserver function on an
// instance of SyncChangeProcessor.
class LocalChangeObserver {
 public:
  virtual ~LocalChangeObserver() {}
  // Function called to notify observer of the local change. current_entry
  // should reflect the state of the entry *before* change is applied.
  virtual void OnLocalChange(const syncable::Entry* current_entry,
                             const SyncChange& change) = 0;
};
}  // namespace syncer

#endif  // COMPONENTS_SYNC_MODEL_LOCAL_CHANGE_OBSERVER_H_
