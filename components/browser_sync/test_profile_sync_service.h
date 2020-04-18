// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_SYNC_TEST_PROFILE_SYNC_SERVICE_H_
#define COMPONENTS_BROWSER_SYNC_TEST_PROFILE_SYNC_SERVICE_H_

#include "base/macros.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/base/weak_handle.h"
#include "components/sync/driver/data_type_manager.h"
#include "components/sync/js/js_event_handler.h"
#include "components/sync/test/engine/test_id_factory.h"

namespace syncer {
class SyncPrefs;
}  // namespace syncer

namespace browser_sync {

class TestProfileSyncService : public ProfileSyncService {
 public:
  explicit TestProfileSyncService(InitParams init_params);

  ~TestProfileSyncService() override;

  void OnConfigureDone(
      const syncer::DataTypeManager::ConfigureResult& result) override;

  // We implement our own version to avoid some DCHECKs.
  syncer::UserShare* GetUserShare() const override;

  syncer::TestIdFactory* id_factory();

  // Raise visibility to ease testing.
  using ProfileSyncService::NotifyObservers;

  syncer::SyncPrefs* sync_prefs() { return &sync_prefs_; }

 protected:
  // Return null handle to use in backend initialization to avoid receiving
  // js messages on UI loop when it's being destroyed, which are not deleted
  // and cause memory leak in test.
  syncer::WeakHandle<syncer::JsEventHandler> GetJsEventHandler() override;

 private:
  syncer::TestIdFactory id_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestProfileSyncService);
};

}  // namespace browser_sync

#endif  // COMPONENTS_BROWSER_SYNC_TEST_PROFILE_SYNC_SERVICE_H_
