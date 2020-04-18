// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_SESSIONS_SESSION_DATA_TYPE_CONTROLLER_H_
#define COMPONENTS_SYNC_SESSIONS_SESSION_DATA_TYPE_CONTROLLER_H_

#include <memory>

#include "base/macros.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync/device_info/local_device_info_provider.h"
#include "components/sync/driver/async_directory_type_controller.h"

namespace sync_sessions {

// Overrides StartModels to avoid sync contention with sessions during
// a session restore operation at startup and to wait for the local
// device info to become available.
class SessionDataTypeController : public syncer::AsyncDirectoryTypeController {
 public:
  // |dump_stack| is called when an unrecoverable error occurs.
  SessionDataTypeController(const base::Closure& dump_stack,
                            syncer::SyncClient* sync_client,
                            syncer::LocalDeviceInfoProvider* local_device,
                            const char* history_disabled_pref_name);
  ~SessionDataTypeController() override;

  // AsyncDirectoryTypeController implementation.
  bool StartModels() override;
  void StopModels() override;
  bool ReadyForStart() const override;

  // Called when asynchronous session restore has completed.
  void OnSessionRestoreComplete();

 private:
  bool IsWaiting();
  void MaybeCompleteLoading();
  void OnLocalDeviceInfoInitialized();
  void OnSavingBrowserHistoryPrefChanged();

  syncer::SyncClient* const sync_client_;

  syncer::LocalDeviceInfoProvider* const local_device_;
  std::unique_ptr<syncer::LocalDeviceInfoProvider::Subscription> subscription_;

  // Name of the pref that indicates whether saving history is disabled.
  const char* history_disabled_pref_name_;

  // Flags that indicate the reason for pending loading models.
  bool waiting_on_session_restore_;
  bool waiting_on_local_device_info_;

  PrefChangeRegistrar pref_registrar_;

  DISALLOW_COPY_AND_ASSIGN(SessionDataTypeController);
};

}  // namespace sync_sessions

#endif  // COMPONENTS_SYNC_SESSIONS_SESSION_DATA_TYPE_CONTROLLER_H_
