// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/touch_uma/touch_uma.h"

#include "ash/touch/touch_uma.h"

// static
void TouchUMA::RecordGestureAction(GestureActionType action) {
  ash::GestureActionType type = ash::GESTURE_UNKNOWN;
  switch (action) {
    case GESTURE_TABSWITCH_TAP:
      type = ash::GESTURE_TABSWITCH_TAP;
      break;
    case GESTURE_TABNOSWITCH_TAP:
      type = ash::GESTURE_TABNOSWITCH_TAP;
      break;
    case GESTURE_TABCLOSE_TAP:
      type = ash::GESTURE_TABCLOSE_TAP;
      break;
    case GESTURE_NEWTAB_TAP:
      type = ash::GESTURE_NEWTAB_TAP;
      break;
    case GESTURE_ROOTVIEWTOP_TAP:
      type = ash::GESTURE_ROOTVIEWTOP_TAP;
      break;
  }
  ash::TouchUMA::GetInstance()->RecordGestureAction(type);
}
