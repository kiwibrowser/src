// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/sync_manager_factory_for_profile_sync_test.h"

#include "components/sync/engine_impl/sync_manager_for_profile_sync_test.h"

namespace syncer {

SyncManagerFactoryForProfileSyncTest::SyncManagerFactoryForProfileSyncTest(
    base::OnceClosure init_callback)
    : init_callback_(std::move(init_callback)) {}

SyncManagerFactoryForProfileSyncTest::~SyncManagerFactoryForProfileSyncTest() {}

std::unique_ptr<SyncManager>
SyncManagerFactoryForProfileSyncTest::CreateSyncManager(
    const std::string& name) {
  return std::unique_ptr<SyncManager>(
      new SyncManagerForProfileSyncTest(name, std::move(init_callback_)));
}

}  // namespace syncer
