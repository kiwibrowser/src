// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/sync_manager_factory.h"

#include "components/sync/engine_impl/sync_manager_impl.h"

namespace syncer {

SyncManagerFactory::SyncManagerFactory() {}

SyncManagerFactory::~SyncManagerFactory() {}

std::unique_ptr<SyncManager> SyncManagerFactory::CreateSyncManager(
    const std::string& name) {
  return std::unique_ptr<SyncManager>(new SyncManagerImpl(name));
}

}  // namespace syncer
