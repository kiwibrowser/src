// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_HOST_H_
#define CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_HOST_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/fullscreen_control/fullscreen_control_popup.h"
#include "ui/events/event_handler.h"

class ExclusiveAccessContext;
class ExclusiveAccessBubbleViewsContext;

namespace ui {
class GestureEvent;
class MouseEvent;
class TouchEvent;
}  // namespace ui

// This is a UI component that helps user exit fullscreen without using a
// keyboard. It drops an "X" button from the top of the screen when user moves
// cursor to the top or long-press on the screen. Pressing that button will exit
// fullscreen.
// This UI is also used as a visual progress indicator when keyboard lock
// requires user to press-and-hold ESC key to exit fullscreen.
class FullscreenControlHost : public ui::EventHandler {
 public:
  FullscreenControlHost(
      ExclusiveAccessContext* exclusive_access_context,
      ExclusiveAccessBubbleViewsContext* bubble_views_context);
  ~FullscreenControlHost() override;

  static bool IsFullscreenExitUIEnabled();

  // ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnTouchEvent(ui::TouchEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  void Hide(bool animate);

  bool IsVisible() const;

 private:
  friend class FullscreenControlViewTest;

  // Ensures symmetric input show and hide (e.g. a touch show is hidden by
  // touch).
  enum class InputEntryMethod {
    NOT_ACTIVE,  // The view is hidden.
    KEYBOARD,    // A key event caused the view to show.
    MOUSE,       // A mouse event caused the view to show.
    TOUCH,       // A touch event caused the view to show.
  };

  FullscreenControlPopup* GetPopup();
  bool IsPopupCreated() const;
  bool IsAnimating() const;
  void ShowForInputEntryMethod(InputEntryMethod input_entry_method);
  void OnVisibilityChanged();
  void StartPopupTimeout(InputEntryMethod expected_input_method);
  void OnPopupTimeout(InputEntryMethod expected_input_method);
  bool IsExitUiNeeded();
  float CalculateCursorBufferHeight() const;

  InputEntryMethod input_entry_method_ = InputEntryMethod::NOT_ACTIVE;

  bool in_mouse_cooldown_mode_ = false;

  ExclusiveAccessContext* const exclusive_access_context_;
  ExclusiveAccessBubbleViewsContext* const bubble_views_context_;

  std::unique_ptr<FullscreenControlPopup> fullscreen_control_popup_;

  base::OneShotTimer popup_timeout_timer_;
  base::OneShotTimer key_press_delay_timer_;

  // Used to allow tests to wait for popup visibility changes.
  base::OnceClosure on_popup_visibility_changed_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenControlHost);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_HOST_H_
