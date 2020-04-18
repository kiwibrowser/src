// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_ACCESSIBILITY_OBSERVER_H_
#define ASH_ACCESSIBILITY_ACCESSIBILITY_OBSERVER_H_

#include "ash/ash_export.h"

namespace ash {

class ASH_EXPORT AccessibilityObserver {
 public:
  virtual ~AccessibilityObserver() = default;

  // Called when any accessibility status changes.
  virtual void OnAccessibilityStatusChanged() = 0;
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_ACCESSIBILITY_OBSERVER_H_
