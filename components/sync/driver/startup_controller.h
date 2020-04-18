// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_STARTUP_CONTROLLER_H_
#define COMPONENTS_SYNC_DRIVER_STARTUP_CONTROLLER_H_

#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/sync/base/model_type.h"

namespace syncer {

class SyncPrefs;

// This class is used by ProfileSyncService to manage all logic and state
// pertaining to initialization of the SyncEngine.
class StartupController {
 public:
  StartupController(const SyncPrefs* sync_prefs,
                    base::Callback<bool()> can_start,
                    base::Closure start_engine);
  ~StartupController();

  // Starts up sync if it is requested by the user and preconditions are met.
  void TryStart();

  // Same as TryStart() above, but bypasses deferred startup and the first setup
  // complete check.
  void TryStartImmediately();

  // Called when a datatype (SyncableService) has a need for sync to start
  // ASAP, presumably because a local change event has occurred but we're
  // still in deferred start mode, meaning the SyncableService hasn't been
  // told to MergeDataAndStartSyncing yet.
  // It is expected that |type| is a currently active datatype.
  void OnDataTypeRequestsSyncStartup(ModelType type);

  // Prepares this object for a new attempt to start sync, forgetting
  // whether or not preconditions were previously met.
  // NOTE: This resets internal state managed by this class, but does not
  // touch values that are explicitly set and reset by higher layers to
  // tell this class whether a setup UI dialog is being shown to the user.
  // See setup_in_progress_.
  void Reset(const ModelTypeSet registered_types);

  // Sets the setup in progress flag and tries to start sync if it's true.
  void SetSetupInProgress(bool setup_in_progress);

  bool IsSetupInProgress() const { return setup_in_progress_; }
  base::Time start_engine_time() const { return start_engine_time_; }
  std::string GetEngineInitializationStateString() const;

  void OverrideFallbackTimeoutForTest(const base::TimeDelta& timeout);

 private:
  enum StartUpDeferredOption { STARTUP_DEFERRED, STARTUP_IMMEDIATE };

  void StartUp(StartUpDeferredOption deferred_option);
  void OnFallbackStartupTimerExpired();

  // Records time spent in deferred state with UMA histograms.
  void RecordTimeDeferred();

  // If true, will bypass the FirstSetupComplete check when triggering sync
  // startup.
  bool bypass_setup_complete_;

  // True if we should start sync ASAP because either a SyncableService has
  // requested it, or we're done waiting for a sign and decided to go ahead.
  bool received_start_request_;

  // The time that StartUp() is called. This is used to calculate time spent
  // in the deferred state; that is, after StartUp and before invoking the
  // start_engine_ callback.
  base::Time start_up_time_;

  // If |true|, there is setup UI visible so we should not start downloading
  // data types.
  // Note: this is explicitly controlled by higher layers (UI) and is meant to
  // reflect what the UI claims the setup state to be. Therefore, only set this
  // due to explicit requests to do so via SetSetupInProgress.
  bool setup_in_progress_;

  const SyncPrefs* sync_prefs_;

  // A function that can be invoked repeatedly to determine whether sync can be
  // started. |start_engine_| should not be invoked unless this returns true.
  base::Callback<bool()> can_start_;

  // The callback we invoke when it's time to call expensive
  // startup routines for the sync engine.
  base::Closure start_engine_;

  // The time at which we invoked the start_engine_ callback.
  base::Time start_engine_time_;

  base::TimeDelta fallback_timeout_;

  // Used to compute preferred_types from SyncPrefs as-needed.
  ModelTypeSet registered_types_;

  base::WeakPtrFactory<StartupController> weak_factory_;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_STARTUP_CONTROLLER_H_
