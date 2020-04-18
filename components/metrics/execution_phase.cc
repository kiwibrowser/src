// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/execution_phase.h"

#include "build/build_config.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

#if defined(OS_WIN)
#include "components/browser_watcher/stability_data_names.h"
#include "components/browser_watcher/stability_debugging.h"
#endif  // defined(OS_WIN)

namespace metrics {

ExecutionPhaseManager::ExecutionPhaseManager(PrefService* local_state)
    : local_state_(local_state) {}

ExecutionPhaseManager::~ExecutionPhaseManager() {}

// static
void ExecutionPhaseManager::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(
      prefs::kStabilityExecutionPhase,
      static_cast<int>(ExecutionPhase::UNINITIALIZED_PHASE));
}

// static
ExecutionPhase ExecutionPhaseManager::execution_phase_ =
    ExecutionPhase::UNINITIALIZED_PHASE;

void ExecutionPhaseManager::SetExecutionPhase(ExecutionPhase execution_phase) {
  DCHECK(execution_phase != ExecutionPhase::START_METRICS_RECORDING ||
         execution_phase_ == ExecutionPhase::UNINITIALIZED_PHASE);
  execution_phase_ = execution_phase;
  local_state_->SetInteger(prefs::kStabilityExecutionPhase,
                           static_cast<int>(execution_phase_));
#if defined(OS_WIN)
  browser_watcher::SetStabilityDataInt(
      browser_watcher::kStabilityExecutionPhase,
      static_cast<int>(execution_phase_));
#endif  // defined(OS_WIN)
}

ExecutionPhase ExecutionPhaseManager::GetExecutionPhase() {
  // TODO(rtenneti): On windows, consider saving/getting execution_phase from
  // the registry.
  return static_cast<ExecutionPhase>(
      local_state_->GetInteger(prefs::kStabilityExecutionPhase));
}

#if defined(OS_ANDROID) || defined(OS_IOS)
void ExecutionPhaseManager::OnAppEnterBackground() {
  // Note: the in-memory ExecutionPhaseManager::execution_phase_ is not updated.
  local_state_->SetInteger(prefs::kStabilityExecutionPhase,
                           static_cast<int>(ExecutionPhase::SHUTDOWN_COMPLETE));
}

void ExecutionPhaseManager::OnAppEnterForeground() {
  // Restore prefs value altered by OnEnterBackground.
  local_state_->SetInteger(prefs::kStabilityExecutionPhase,
                           static_cast<int>(execution_phase_));
}
#endif  // defined(OS_ANDROID) || defined(OS_IOS)

}  // namespace metrics
