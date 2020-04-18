// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_sessions/session_data_type_controller.h"

#include <set>

#include "base/threading/thread_task_runner_handle.h"
#include "components/prefs/pref_service.h"
#include "components/sync/driver/sync_client.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "components/sync_sessions/synced_window_delegate.h"
#include "components/sync_sessions/synced_window_delegates_getter.h"

namespace sync_sessions {

SessionDataTypeController::SessionDataTypeController(
    const base::Closure& dump_stack,
    syncer::SyncClient* sync_client,
    syncer::LocalDeviceInfoProvider* local_device,
    const char* history_disabled_pref_name)
    : AsyncDirectoryTypeController(syncer::SESSIONS,
                                   dump_stack,
                                   sync_client,
                                   syncer::GROUP_UI,
                                   base::ThreadTaskRunnerHandle::Get()),
      sync_client_(sync_client),
      local_device_(local_device),
      history_disabled_pref_name_(history_disabled_pref_name),
      waiting_on_session_restore_(false),
      waiting_on_local_device_info_(false) {
  DCHECK(local_device_);
  pref_registrar_.Init(sync_client_->GetPrefService());
  pref_registrar_.Add(
      history_disabled_pref_name_,
      base::Bind(&SessionDataTypeController::OnSavingBrowserHistoryPrefChanged,
                 base::AsWeakPtr(this)));
}

SessionDataTypeController::~SessionDataTypeController() {}

bool SessionDataTypeController::StartModels() {
  DCHECK(CalledOnValidThread());
  SyncedWindowDelegatesGetter* synced_window_getter =
      sync_client_->GetSyncSessionsClient()->GetSyncedWindowDelegatesGetter();
  SyncedWindowDelegatesGetter::SyncedWindowDelegateMap windows =
      synced_window_getter->GetSyncedWindowDelegates();
  for (const auto& window_iter_pair : windows) {
    if (window_iter_pair.second->IsSessionRestoreInProgress()) {
      waiting_on_session_restore_ = true;
      break;
    }
  }

  if (!local_device_->GetLocalDeviceInfo()) {
    subscription_ = local_device_->RegisterOnInitializedCallback(
        base::Bind(&SessionDataTypeController::OnLocalDeviceInfoInitialized,
                   base::AsWeakPtr(this)));
    waiting_on_local_device_info_ = true;
  }

  return !IsWaiting();
}

void SessionDataTypeController::StopModels() {
  DCHECK(CalledOnValidThread());
  subscription_.reset();
}

bool SessionDataTypeController::ReadyForStart() const {
  DCHECK(CalledOnValidThread());
  return !sync_client_->GetPrefService()->GetBoolean(
      history_disabled_pref_name_);
}

void SessionDataTypeController::OnSessionRestoreComplete() {
  DCHECK(CalledOnValidThread());
  waiting_on_session_restore_ = false;
  MaybeCompleteLoading();
}

bool SessionDataTypeController::IsWaiting() {
  return waiting_on_session_restore_ || waiting_on_local_device_info_;
}

void SessionDataTypeController::MaybeCompleteLoading() {
  if (state() == MODEL_STARTING && !IsWaiting()) {
    OnModelLoaded();
  }
}

void SessionDataTypeController::OnLocalDeviceInfoInitialized() {
  DCHECK(CalledOnValidThread());
  subscription_.reset();

  waiting_on_local_device_info_ = false;
  MaybeCompleteLoading();
}

void SessionDataTypeController::OnSavingBrowserHistoryPrefChanged() {
  DCHECK(CalledOnValidThread());
  if (sync_client_->GetPrefService()->GetBoolean(history_disabled_pref_name_)) {
    // If history and tabs persistence is turned off then generate an
    // unrecoverable error. SESSIONS won't be a registered type on the next
    // Chrome restart.
    if (state() != NOT_RUNNING && state() != STOPPING) {
      syncer::SyncError error(
          FROM_HERE, syncer::SyncError::DATATYPE_POLICY_ERROR,
          "History and tab saving is now disabled by policy.",
          syncer::SESSIONS);
      CreateErrorHandler()->OnUnrecoverableError(error);
    }
  }
}

}  // namespace sync_sessions
