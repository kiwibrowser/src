// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_SCREEN_SECURITY_SCREEN_CAPTURE_OBSERVER_H_
#define ASH_SYSTEM_SCREEN_SECURITY_SCREEN_CAPTURE_OBSERVER_H_

#include "base/callback.h"
#include "base/strings/string16.h"

namespace ash {

class ScreenCaptureObserver {
 public:
  // Called when screen capture is started.
  virtual void OnScreenCaptureStart(
      const base::Closure& stop_callback,
      const base::string16& screen_capture_status) = 0;

  // Called when screen capture is stopped.
  virtual void OnScreenCaptureStop() = 0;

 protected:
  virtual ~ScreenCaptureObserver() {}
};

}  // namespace ash

#endif  // ASH_SYSTEM_SCREEN_SECURITY_SCREEN_CAPTURE_OBSERVER_H_
