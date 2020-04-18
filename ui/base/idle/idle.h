// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IDLE_IDLE_H_
#define UI_BASE_IDLE_IDLE_H_

#include "base/callback.h"
#include "build/build_config.h"
#include "ui/base/ui_base_export.h"

namespace ui {

enum IdleState {
  IDLE_STATE_ACTIVE = 0,
  IDLE_STATE_IDLE = 1,    // No activity within threshold.
  IDLE_STATE_LOCKED = 2,  // Only available on supported systems.
  IDLE_STATE_UNKNOWN = 3  // Used when waiting for the Idle state or in error
                          // conditions
};

// For MacOSX, InitIdleMonitor needs to be called first to setup the monitor.
#if defined(OS_MACOSX)
UI_BASE_EXPORT void InitIdleMonitor();
#endif

typedef base::Callback<void(IdleState)> IdleCallback;
typedef base::Callback<void(int)> IdleTimeCallback;

// Calculate the Idle state and notify the callback. |idle_threshold| is the
// amount of time (in seconds) before considered idle. |notify| is
// asynchronously called on some platforms.
UI_BASE_EXPORT void CalculateIdleState(int idle_threshold, IdleCallback notify);

// Calculate Idle time in seconds and notify the callback
UI_BASE_EXPORT void CalculateIdleTime(IdleTimeCallback notify);

// Checks synchronously if Idle state is IDLE_STATE_LOCKED.
UI_BASE_EXPORT bool CheckIdleStateIsLocked();

}  // namespace ui

#endif  // UI_BASE_IDLE_IDLE_H_
