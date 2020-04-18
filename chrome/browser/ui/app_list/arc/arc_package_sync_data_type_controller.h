// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_ARC_ARC_PACKAGE_SYNC_DATA_TYPE_CONTROLLER_H_
#define CHROME_BROWSER_UI_APP_LIST_ARC_ARC_PACKAGE_SYNC_DATA_TYPE_CONTROLLER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "components/sync/driver/async_directory_type_controller.h"
#include "components/sync/driver/data_type_controller.h"

class Profile;

namespace syncer {
class SyncClient;
}

// A DataTypeController for arc package sync datatypes, which enables or
// disables these types based on whether ArcAppInstance is ready.
class ArcPackageSyncDataTypeController
    : public syncer::AsyncDirectoryTypeController,
      public ArcAppListPrefs::Observer,
      public arc::ArcSessionManager::Observer {
 public:
  // |dump_stack| is called when an unrecoverable error occurs.
  ArcPackageSyncDataTypeController(syncer::ModelType type,
                                   const base::Closure& dump_stack,
                                   syncer::SyncClient* sync_client,
                                   Profile* profile);
  ~ArcPackageSyncDataTypeController() override;

  // AsyncDirectoryTypeController implementation.
  bool ReadyForStart() const override;
  bool StartModels() override;
  void StopModels() override;

 private:
  // ArcAppListPrefs::Observer:
  void OnPackageListInitialRefreshed() override;

  // ArcSessionManager::Observer:
  void OnArcPlayStoreEnabledChanged(bool enabled) override;
  void OnArcInitialStart() override;

  void EnableDataType();

  // Returns true if user enables app sync.
  bool ShouldSyncArc() const;

  bool model_normal_start_ = true;

  Profile* const profile_;

  DISALLOW_COPY_AND_ASSIGN(ArcPackageSyncDataTypeController);
};
#endif  // CHROME_BROWSER_UI_APP_LIST_ARC_ARC_PACKAGE_SYNC_DATA_TYPE_CONTROLLER_H_
