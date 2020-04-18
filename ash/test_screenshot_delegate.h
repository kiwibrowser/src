// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_SCREENSHOT_DELEGATE_H_
#define ASH_TEST_SCREENSHOT_DELEGATE_H_

#include "ash/screenshot_delegate.h"
#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {

class TestScreenshotDelegate : public ScreenshotDelegate {
 public:
  TestScreenshotDelegate();
  ~TestScreenshotDelegate() override;

  // Overridden from ScreenshotDelegate:
  void HandleTakeScreenshotForAllRootWindows() override;
  void HandleTakePartialScreenshot(aura::Window* window,
                                   const gfx::Rect& rect) override;
  void HandleTakeWindowScreenshot(aura::Window* window) override;
  bool CanTakeScreenshot() override;

  int handle_take_screenshot_count() const {
    return handle_take_screenshot_count_;
  }

  int handle_take_partial_screenshot_count() const {
    return handle_take_partial_screenshot_count_;
  }

  void reset_handle_take_screenshot_count() {
    handle_take_screenshot_count_ = 0;
  }

  const gfx::Rect& last_rect() const { return last_rect_; }

  const aura::Window* GetSelectedWindowAndReset();

  void set_can_take_screenshot(bool can_take_screenshot) {
    can_take_screenshot_ = can_take_screenshot;
  }

 private:
  int handle_take_screenshot_count_ = 0;
  int handle_take_partial_screenshot_count_ = 0;
  gfx::Rect last_rect_;
  bool can_take_screenshot_ = true;
  aura::Window* selected_window_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TestScreenshotDelegate);
};

}  // namespace ash

#endif  // ASH_TEST_SCREENSHOT_DELEGATE_H_
