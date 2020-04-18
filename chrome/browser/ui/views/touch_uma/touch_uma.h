// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TOUCH_UMA_TOUCH_UMA_H_
#define CHROME_BROWSER_UI_VIEWS_TOUCH_UMA_TOUCH_UMA_H_

#include "base/macros.h"

class TouchUMA {
 public:
  enum GestureActionType {
    GESTURE_TABSWITCH_TAP,
    GESTURE_TABNOSWITCH_TAP,
    GESTURE_TABCLOSE_TAP,
    GESTURE_NEWTAB_TAP,
    GESTURE_ROOTVIEWTOP_TAP,
  };

  static void RecordGestureAction(GestureActionType action);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TouchUMA);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TOUCH_UMA_TOUCH_UMA_H_
