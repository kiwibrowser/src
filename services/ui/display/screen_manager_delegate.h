// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_DISPLAY_SCREEN_MANAGER_DELEGATE_H_
#define SERVICES_UI_DISPLAY_SCREEN_MANAGER_DELEGATE_H_

#include <stdint.h>

namespace display {

class Display;
struct ViewportMetrics;

// The ScreenManagerDelegate will be informed of changes to the display or
// screen state by ScreenManager.
class ScreenManagerDelegate {
 public:
  // Called when a display is added.
  virtual void OnDisplayAdded(const display::Display& display,
                              const ViewportMetrics& metrics) = 0;

  // Called when a display is modified.
  virtual void OnDisplayModified(const display::Display& display,
                                 const ViewportMetrics& metrics) = 0;

  // Called when a display is removed.
  virtual void OnDisplayRemoved(int64_t display_id) = 0;

  // Called when the primary display is changed.
  virtual void OnPrimaryDisplayChanged(int64_t primary_display_id) = 0;

 protected:
  virtual ~ScreenManagerDelegate() {}
};

}  // namespace display

#endif  // SERVICES_UI_DISPLAY_SCREEN_MANAGER_DELEGATE_H_
