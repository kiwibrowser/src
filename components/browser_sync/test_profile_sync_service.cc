// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_sync/test_profile_sync_service.h"

#include <utility>

#include "base/run_loop.h"

namespace browser_sync {

syncer::TestIdFactory* TestProfileSyncService::id_factory() {
  return &id_factory_;
}

syncer::WeakHandle<syncer::JsEventHandler>
TestProfileSyncService::GetJsEventHandler() {
  return syncer::WeakHandle<syncer::JsEventHandler>();
}

TestProfileSyncService::TestProfileSyncService(
    ProfileSyncService::InitParams init_params)
    : ProfileSyncService(std::move(init_params)) {}

TestProfileSyncService::~TestProfileSyncService() {}

void TestProfileSyncService::OnConfigureDone(
    const syncer::DataTypeManager::ConfigureResult& result) {
  ProfileSyncService::OnConfigureDone(result);
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

syncer::UserShare* TestProfileSyncService::GetUserShare() const {
  return engine_->GetUserShare();
}

}  // namespace browser_sync
