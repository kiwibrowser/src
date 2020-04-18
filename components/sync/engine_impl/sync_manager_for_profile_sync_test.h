// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_IMPL_SYNC_MANAGER_FOR_PROFILE_SYNC_TEST_H_
#define COMPONENTS_SYNC_ENGINE_IMPL_SYNC_MANAGER_FOR_PROFILE_SYNC_TEST_H_

#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "components/sync/engine_impl/sync_manager_impl.h"

namespace syncer {

// This class is used to help implement the TestProfileSyncService.
// Those tests try to test sync without instantiating a real backend.
class SyncManagerForProfileSyncTest : public SyncManagerImpl {
 public:
  SyncManagerForProfileSyncTest(std::string name,
                                base::OnceClosure init_callback);
  ~SyncManagerForProfileSyncTest() override;
  void NotifyInitializationSuccess() override;

 private:
  base::OnceClosure init_callback_;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_IMPL_SYNC_MANAGER_FOR_PROFILE_SYNC_TEST_H_
