// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_LINUX_UI_WINDOW_BUTTON_ORDER_OBSERVER_H_
#define UI_VIEWS_LINUX_UI_WINDOW_BUTTON_ORDER_OBSERVER_H_

#include <vector>

#include "ui/views/window/frame_buttons.h"

namespace views {

// Observer interface to receive the ordering of the min,max,close buttons.
class WindowButtonOrderObserver {
 public:
  // Called when first added to the LinuxUI class, or on a system-wide
  // configuration event.
  virtual void OnWindowButtonOrderingChange(
      const std::vector<views::FrameButton>& leading_buttons,
      const std::vector<views::FrameButton>& trailing_buttons) = 0;

 protected:
  virtual ~WindowButtonOrderObserver() {}
};

}  // namespace views

#endif  // UI_VIEWS_LINUX_UI_WINDOW_BUTTON_ORDER_OBSERVER_H_
