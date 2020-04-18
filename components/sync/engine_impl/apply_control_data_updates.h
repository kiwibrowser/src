// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_APPLY_CONTROL_DATA_UPDATES_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_APPLY_CONTROL_DATA_UPDATES_H_

namespace syncer {

class Cryptographer;

namespace syncable {
class Directory;
class MutableEntry;
class WriteTransaction;
}

void ApplyControlDataUpdates(syncable::Directory* dir);
void ApplyNigoriUpdate(syncable::WriteTransaction* trans,
                       syncable::MutableEntry* const entry,
                       Cryptographer* cryptographer);
void ApplyControlUpdate(syncable::WriteTransaction* const trans,
                        syncable::MutableEntry* const entry,
                        Cryptographer* cryptographer);

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_APPLY_CONTROL_DATA_UPDATES_H_
