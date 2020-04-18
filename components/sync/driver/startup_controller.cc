// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/startup_controller.h"

#include "base/command_line.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync/driver/sync_driver_switches.h"

namespace syncer {

namespace {

// The amount of time we'll wait to initialize sync if no data type triggers
// initialization via a StartSyncFlare.
const int kDeferredInitFallbackSeconds = 10;

// Enum (for UMA, primarily) defining different events that cause us to
// exit the "deferred" state of initialization and invoke start_engine.
enum DeferredInitTrigger {
  // We have received a signal from a SyncableService requesting that sync
  // starts as soon as possible.
  TRIGGER_DATA_TYPE_REQUEST,
  // No data type requested sync to start and our fallback timer expired.
  TRIGGER_FALLBACK_TIMER,
  MAX_TRIGGER_VALUE
};

}  // namespace

StartupController::StartupController(const SyncPrefs* sync_prefs,
                                     base::Callback<bool()> can_start,
                                     base::Closure start_engine)
    : bypass_setup_complete_(false),
      received_start_request_(false),
      setup_in_progress_(false),
      sync_prefs_(sync_prefs),
      can_start_(can_start),
      start_engine_(start_engine),
      fallback_timeout_(
          base::TimeDelta::FromSeconds(kDeferredInitFallbackSeconds)),
      weak_factory_(this) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSyncDeferredStartupTimeoutSeconds)) {
    int timeout = kDeferredInitFallbackSeconds;
    if (base::StringToInt(
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                switches::kSyncDeferredStartupTimeoutSeconds),
            &timeout)) {
      DCHECK_GE(timeout, 0);
      DVLOG(2) << "Sync StartupController overriding startup timeout to "
               << timeout << " seconds.";
      fallback_timeout_ = base::TimeDelta::FromSeconds(timeout);
    }
  }
}

StartupController::~StartupController() {}

void StartupController::Reset(const ModelTypeSet registered_types) {
  received_start_request_ = false;
  bypass_setup_complete_ = false;
  start_up_time_ = base::Time();
  start_engine_time_ = base::Time();
  // Don't let previous timers affect us post-reset.
  weak_factory_.InvalidateWeakPtrs();
  registered_types_ = registered_types;
}

void StartupController::SetSetupInProgress(bool setup_in_progress) {
  setup_in_progress_ = setup_in_progress;
  if (setup_in_progress_) {
    TryStart();
  }
}

void StartupController::StartUp(StartUpDeferredOption deferred_option) {
  const bool first_start = start_up_time_.is_null();
  if (first_start) {
    start_up_time_ = base::Time::Now();
  }

  if (deferred_option == STARTUP_DEFERRED &&
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSyncDisableDeferredStartup) &&
      sync_prefs_->GetPreferredDataTypes(registered_types_).Has(SESSIONS)) {
    if (first_start) {
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&StartupController::OnFallbackStartupTimerExpired,
                     weak_factory_.GetWeakPtr()),
          fallback_timeout_);
    }
    return;
  }

  if (start_engine_time_.is_null()) {
    start_engine_time_ = base::Time::Now();
    start_engine_.Run();
  }
}

void StartupController::OverrideFallbackTimeoutForTest(
    const base::TimeDelta& timeout) {
  fallback_timeout_ = timeout;
}

void StartupController::TryStart() {
  if (!can_start_.Run()) {
    return;
  }

  // For performance reasons, defer the heavy lifting for sync init unless:
  //
  // - a datatype has requested an immediate start of sync, or
  // - sync needs to start up the engine immediately to provide control state
  //   and encryption information to the UI.
  // Do not start up the sync engine if setup has not completed and isn't
  // in progress, unless told to otherwise.
  if (setup_in_progress_) {
    StartUp(STARTUP_IMMEDIATE);
  } else if (sync_prefs_->IsFirstSetupComplete() || bypass_setup_complete_) {
    StartUp(received_start_request_ ? STARTUP_IMMEDIATE : STARTUP_DEFERRED);
  }
}

void StartupController::TryStartImmediately() {
  received_start_request_ = true;
  bypass_setup_complete_ = true;
  TryStart();
}

void StartupController::RecordTimeDeferred() {
  DCHECK(!start_up_time_.is_null());
  base::TimeDelta time_deferred = base::Time::Now() - start_up_time_;
  UMA_HISTOGRAM_CUSTOM_TIMES("Sync.Startup.TimeDeferred2", time_deferred,
                             base::TimeDelta::FromSeconds(0),
                             base::TimeDelta::FromMinutes(2), 60);
}

void StartupController::OnFallbackStartupTimerExpired() {
  DCHECK(!base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kSyncDisableDeferredStartup));

  if (!start_engine_time_.is_null())
    return;

  DVLOG(2) << "Sync deferred init fallback timer expired, starting engine.";
  RecordTimeDeferred();
  UMA_HISTOGRAM_ENUMERATION("Sync.Startup.DeferredInitTrigger",
                            TRIGGER_FALLBACK_TIMER, MAX_TRIGGER_VALUE);
  received_start_request_ = true;
  TryStart();
}

std::string StartupController::GetEngineInitializationStateString() const {
  if (!start_engine_time_.is_null())
    return "Started";
  else if (!start_up_time_.is_null())
    return "Deferred";
  else
    return "Not started";
}

void StartupController::OnDataTypeRequestsSyncStartup(ModelType type) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSyncDisableDeferredStartup)) {
    DVLOG(2) << "Ignoring data type request for sync startup: "
             << ModelTypeToString(type);
    return;
  }

  if (!start_engine_time_.is_null())
    return;

  DVLOG(2) << "Data type requesting sync startup: " << ModelTypeToString(type);
  // Measure the time spent waiting for init and the type that triggered it.
  // We could measure the time spent deferred on a per-datatype basis, but
  // for now this is probably sufficient.
  // TODO(wychen): enum uma should be strongly typed. crbug.com/661401
  UMA_HISTOGRAM_ENUMERATION("Sync.Startup.TypeTriggeringInit",
                            ModelTypeToHistogramInt(type),
                            static_cast<int>(MODEL_TYPE_COUNT));
  if (!start_up_time_.is_null()) {
    RecordTimeDeferred();
    UMA_HISTOGRAM_ENUMERATION("Sync.Startup.DeferredInitTrigger",
                              TRIGGER_DATA_TYPE_REQUEST, MAX_TRIGGER_VALUE);
  }
  received_start_request_ = true;
  TryStart();
}

}  // namespace syncer
