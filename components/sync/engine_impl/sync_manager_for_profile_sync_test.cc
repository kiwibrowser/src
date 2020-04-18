// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/sync_manager_for_profile_sync_test.h"

#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/test_user_share.h"
#include "components/sync/syncable/user_share.h"

namespace syncer {

SyncManagerForProfileSyncTest::SyncManagerForProfileSyncTest(
    std::string name,
    base::OnceClosure init_callback)
    : SyncManagerImpl(name), init_callback_(std::move(init_callback)) {}

SyncManagerForProfileSyncTest::~SyncManagerForProfileSyncTest() {}

void SyncManagerForProfileSyncTest::NotifyInitializationSuccess() {
  UserShare* user_share = GetUserShare();
  syncable::Directory* directory = user_share->directory.get();

  if (!init_callback_.is_null())
    std::move(init_callback_).Run();

  ModelTypeSet early_download_types;
  early_download_types.PutAll(ControlTypes());
  early_download_types.PutAll(PriorityUserTypes());
  for (ModelTypeSet::Iterator it = early_download_types.First(); it.Good();
       it.Inc()) {
    if (!directory->InitialSyncEndedForType(it.Get())) {
      TestUserShare::CreateRoot(it.Get(), user_share);
    }
  }

  SyncManagerImpl::NotifyInitializationSuccess();
}

}  // namespace syncer
