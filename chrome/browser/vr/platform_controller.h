// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_PLATFORM_CONTROLLER_H_
#define CHROME_BROWSER_VR_PLATFORM_CONTROLLER_H_

#include "base/time/time.h"

namespace vr {

// This class is not platform-specific.  It will be backed by platform-specific
// controller code, but its interface must be platform-agnostic. For example,
// the enumeration of buttons may map to buttons with different names on a
// different platform's controller, but the functionality must exist. I.e., the
// concept of "the button you press to exit fullscreen / presentation" is
// universal.
class PlatformController {
 public:
  enum ButtonType {
    kButtonHome,
    kButtonMenu,
    kButtonSelect,
  };

  enum SwipeDirection {
    kSwipeDirectionNone,
    kSwipeDirectionLeft,
    kSwipeDirectionRight,
    kSwipeDirectionUp,
    kSwipeDirectionDown,
  };

  enum Handedness {
    kRightHanded,
    kLeftHanded,
  };

  virtual ~PlatformController() {}

  virtual bool IsButtonDown(ButtonType type) const = 0;
  virtual base::TimeTicks GetLastOrientationTimestamp() const = 0;
  virtual base::TimeTicks GetLastTouchTimestamp() const = 0;
  virtual base::TimeTicks GetLastButtonTimestamp() const = 0;
  virtual Handedness GetHandedness() const = 0;
  virtual bool GetRecentered() const = 0;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_PLATFORM_CONTROLLER_H_
