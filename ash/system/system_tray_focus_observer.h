// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_SYSTEM_TRAY_FOCUS_OBSERVER_H_
#define ASH_SYSTEM_SYSTEM_TRAY_FOCUS_OBSERVER_H_

#include "ash/ash_export.h"
#include "base/macros.h"

namespace ash {

// A class that observes system tray related focus events.
class ASH_EXPORT SystemTrayFocusObserver {
 public:
  // Called when focus is about to leave system tray.
  virtual void OnFocusLeavingSystemTray(bool reverse) = 0;

 protected:
  virtual ~SystemTrayFocusObserver() {}
};

}  // namespace ash

#endif  // ASH_SYSTEM_SYSTEM_TRAY_FOCUS_OBSERVER_H_
