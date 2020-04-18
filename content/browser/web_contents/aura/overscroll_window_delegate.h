// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_AURA_OVERSCROLL_WINDOW_DELEGATE_H_
#define CONTENT_BROWSER_WEB_CONTENTS_AURA_OVERSCROLL_WINDOW_DELEGATE_H_

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "content/browser/renderer_host/overscroll_controller.h"
#include "content/browser/web_contents/aura/overscroll_navigation_overlay.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "ui/aura_extra/image_window_delegate.h"

namespace content {

// The window delegate for the overscroll window. This processes UI trackpad and
// touch events and converts them to overscroll event. The delegate destroys
// itself when the window is destroyed.
class CONTENT_EXPORT OverscrollWindowDelegate
    : public aura_extra::ImageWindowDelegate {
 public:
  OverscrollWindowDelegate(OverscrollControllerDelegate* delegate,
                           const gfx::Image& image);

 private:
  FRIEND_TEST_ALL_PREFIXES(OverscrollWindowDelegateTest, BasicOverscrollModes);

  ~OverscrollWindowDelegate() override;

  // Starts the overscroll gesture.
  void StartOverscroll(OverscrollSource source);

  // Resets the overscroll state.
  void ResetOverscroll();

  // Completes or resets the overscroll from the current state.
  void CompleteOrResetOverscroll();

  // Updates the current horizontal overscroll.
  void UpdateOverscroll(float delta_x, OverscrollSource source);

  // Overridden from ui::EventHandler.
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnScrollEvent(ui::ScrollEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  // Delegate to which we forward overscroll events.
  OverscrollControllerDelegate* delegate_;

  // The current overscroll mode.
  OverscrollMode overscroll_mode_;

  // The latest delta_x scroll update.
  float delta_x_;

  // The ratio of overscroll at which we consider the overscroll completed, for
  // touchscreen or touchpad.
  const float complete_threshold_ratio_touchscreen_;
  const float complete_threshold_ratio_touchpad_;

  // The ratio of overscroll at which we consider the overscroll completed for
  // the current touch input.
  float active_complete_threshold_ratio_;

  // The threshold for starting the overscroll gesture, for touchscreen or
  // touchpads.
  const float start_threshold_touchscreen_;
  const float start_threshold_touchpad_;

  // The threshold for starting the overscroll gesture for the current touch
  // input.
  float active_start_threshold_;

  DISALLOW_COPY_AND_ASSIGN(OverscrollWindowDelegate);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_AURA_OVERSCROLL_WINDOW_DELEGATE_H_
