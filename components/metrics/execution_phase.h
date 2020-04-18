// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_EXECUTION_PHASE_H_
#define COMPONENTS_METRICS_EXECUTION_PHASE_H_

#include "base/macros.h"
#include "build/build_config.h"

class PrefService;
class PrefRegistrySimple;

namespace metrics {

enum class ExecutionPhase {
  UNINITIALIZED_PHASE = 0,
  START_METRICS_RECORDING = 100,
  CREATE_PROFILE = 200,
  STARTUP_TIMEBOMB_ARM = 300,
  THREAD_WATCHER_START = 400,
  MAIN_MESSAGE_LOOP_RUN = 500,
  SHUTDOWN_TIMEBOMB_ARM = 600,
  SHUTDOWN_COMPLETE = 700,
};

// Helper class for managing ExecutionPhase state in prefs and memory.
// It's safe to construct temporary objects to perform these operations.
class ExecutionPhaseManager {
 public:
  explicit ExecutionPhaseManager(PrefService* local_state);
  ~ExecutionPhaseManager();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void SetExecutionPhase(ExecutionPhase execution_phase);
  ExecutionPhase GetExecutionPhase();

#if defined(OS_ANDROID) || defined(OS_IOS)
  void OnAppEnterBackground();
  void OnAppEnterForeground();
#endif  // defined(OS_ANDROID) || defined(OS_IOS)

 private:
  // Execution phase the browser is in.
  static ExecutionPhase execution_phase_;

  PrefService* local_state_;

  DISALLOW_COPY_AND_ASSIGN(ExecutionPhaseManager);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_EXECUTION_PHASE_H_
