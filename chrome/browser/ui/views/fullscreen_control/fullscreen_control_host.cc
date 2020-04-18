// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/fullscreen_control/fullscreen_control_host.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/views/exclusive_access_bubble_views.h"
#include "chrome/browser/ui/views/exclusive_access_bubble_views_context.h"
#include "chrome/browser/ui/views/fullscreen_control/fullscreen_control_view.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_features.h"
#include "components/version_info/channel.h"
#include "content/public/common/content_features.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"

namespace {

// +----------------------------+
// |         +-------+          |
// |         |Control|          |
// |         +-------+          |
// |                            | <-- Control.bottom * kExitHeightScaleFactor
// |          Screen            |       Buffer for mouse moves or pointer events
// |                            |       before closing the fullscreen exit
// |                            |       control.
// +----------------------------+
//
// The same value is also used for timeout cooldown.
// This is a common scenario where people play video or present slides and they
// just want to keep their cursor on the top. In this case we timeout the exit
// control so that it doesn't show permanently. The user will then need to move
// the cursor out of the cooldown area and move it back to the top to re-trigger
// the exit UI.
constexpr float kExitHeightScaleFactor = 1.5f;

// +----------------------------+
// |                            |
// |                            |
// |                            | <-- kShowFullscreenExitControlHeight
// |          Screen            |       If a mouse move or pointer event is
// |                            |       above this line, show the fullscreen
// |                            |       exit control.
// |                            |
// +----------------------------+
constexpr float kShowFullscreenExitControlHeight = 3.f;

// Time to wait to hide the popup after it is triggered.
constexpr base::TimeDelta kPopupTimeout = base::TimeDelta::FromSeconds(3);

// Time to wait before showing the popup when the escape key is held.
constexpr base::TimeDelta kKeyPressPopupDelay = base::TimeDelta::FromSeconds(1);

bool IsExitUiEnabled() {
#if defined(OS_MACOSX)
  // Exit UI is unnecessary, since Mac uses the OS fullscreen such that window
  // menu and controls reveal when the cursor is moved to the top.
  return false;
#else
  return base::FeatureList::IsEnabled(features::kFullscreenExitUI);
#endif
}

}  // namespace

FullscreenControlHost::FullscreenControlHost(
    ExclusiveAccessContext* exclusive_access_context,
    ExclusiveAccessBubbleViewsContext* bubble_views_context)
    : exclusive_access_context_(exclusive_access_context),
      bubble_views_context_(bubble_views_context) {}

FullscreenControlHost::~FullscreenControlHost() = default;

// static
bool FullscreenControlHost::IsFullscreenExitUIEnabled() {
  // FullscreenControlHost provides visual feedback for press-and-hold escape
  // gesture to exit fullscreen.  If keyboard lock API is enabled, then we want
  // ensure the control is created and listening to keyboard input.  Otherwise
  // we will only create the control if we need it for touch/mouse events on
  // non-MacOS platforms.
  return base::FeatureList::IsEnabled(features::kKeyboardLockAPI) ||
         IsExitUiEnabled();
}

void FullscreenControlHost::OnKeyEvent(ui::KeyEvent* event) {
  if (event->key_code() != ui::VKEY_ESCAPE ||
      (input_entry_method_ != InputEntryMethod::NOT_ACTIVE &&
       input_entry_method_ != InputEntryMethod::KEYBOARD)) {
    return;
  }

  ExclusiveAccessManager* const exclusive_access_manager =
      bubble_views_context_->GetExclusiveAccessManager();

  // FullscreenControlHost UI is not needed for the keyboard input method in any
  // fullscreen mode except for tab-initiated fullscreen (and only when the user
  // is required to press and hold the escape key to exit).

  // If we are not in tab-initiated fullscreen, then we want to make sure the
  // UI exit bubble is not displayed.  This can occur when:
  // 1.) The user enters browser fullscreen (F11)
  // 2.) The website then enters tab-initiated fullscreen
  // 3.) User performs a press and hold gesture on escape
  //
  // In this case, the fullscreen controller will revert back to browser
  // fullscreen mode but there won't be a fullscreen exit message to trigger
  // the UI cleanup for the exit bubble.  To handle this case, we need to check
  // to make sure the UI is in the right fullscreen mode before proceeding.
  if (!exclusive_access_manager->fullscreen_controller()
           ->IsWindowFullscreenForTabOrPending()) {
    key_press_delay_timer_.Stop();
    if (IsVisible() && input_entry_method_ == InputEntryMethod::KEYBOARD)
      Hide(true);
    return;
  }

  // Note: This logic handles the UI feedback element used when holding down the
  // esc key, however the logic for exiting fullscreen is handled by the
  // KeyboardLockController class.
  if (event->type() == ui::ET_KEY_PRESSED &&
      !key_press_delay_timer_.IsRunning() &&
      exclusive_access_manager->keyboard_lock_controller()
          ->RequiresPressAndHoldEscToExit()) {
    key_press_delay_timer_.Start(
        FROM_HERE, kKeyPressPopupDelay,
        base::Bind(&FullscreenControlHost::ShowForInputEntryMethod,
                   base::Unretained(this), InputEntryMethod::KEYBOARD));
  } else if (event->type() == ui::ET_KEY_RELEASED) {
    key_press_delay_timer_.Stop();
    if (IsVisible() && input_entry_method_ == InputEntryMethod::KEYBOARD)
      Hide(true);
  }
}

void FullscreenControlHost::OnMouseEvent(ui::MouseEvent* event) {
  if (!IsExitUiEnabled())
    return;

  if (event->type() != ui::ET_MOUSE_MOVED || IsAnimating() ||
      (input_entry_method_ != InputEntryMethod::NOT_ACTIVE &&
       input_entry_method_ != InputEntryMethod::MOUSE)) {
    return;
  }

  if (IsExitUiNeeded()) {
    if (IsVisible()) {
      if (event->y() >= CalculateCursorBufferHeight()) {
        Hide(true);
      }
    } else {
      DCHECK_EQ(InputEntryMethod::NOT_ACTIVE, input_entry_method_);
      if (!in_mouse_cooldown_mode_ &&
          event->y() <= kShowFullscreenExitControlHeight) {
        ShowForInputEntryMethod(InputEntryMethod::MOUSE);
      } else if (in_mouse_cooldown_mode_ &&
                 event->y() >= CalculateCursorBufferHeight()) {
        in_mouse_cooldown_mode_ = false;
      }
    }
  } else if (IsVisible()) {
    Hide(true);
  }
}

void FullscreenControlHost::OnTouchEvent(ui::TouchEvent* event) {
  if (input_entry_method_ != InputEntryMethod::TOUCH)
    return;

  DCHECK(IsVisible());

  // Hide the popup if the popup is showing and the user touches outside of the
  // popup.
  if (event->type() == ui::ET_TOUCH_PRESSED && !IsAnimating()) {
    Hide(true);
  }
}

void FullscreenControlHost::OnGestureEvent(ui::GestureEvent* event) {
  if (!IsExitUiEnabled())
    return;

  if (event->type() == ui::ET_GESTURE_LONG_PRESS && IsExitUiNeeded() &&
      !IsVisible()) {
    ShowForInputEntryMethod(InputEntryMethod::TOUCH);
  }
}

void FullscreenControlHost::Hide(bool animate) {
  if (IsPopupCreated()) {
    GetPopup()->Hide(animate);
  }
}

bool FullscreenControlHost::IsVisible() const {
  return IsPopupCreated() && fullscreen_control_popup_->IsVisible();
}

FullscreenControlPopup* FullscreenControlHost::GetPopup() {
  if (!IsPopupCreated()) {
    fullscreen_control_popup_ = std::make_unique<FullscreenControlPopup>(
        bubble_views_context_->GetBubbleParentView(),
        base::BindRepeating(&ExclusiveAccessContext::ExitFullscreen,
                            base::Unretained(exclusive_access_context_)),
        base::BindRepeating(&FullscreenControlHost::OnVisibilityChanged,
                            base::Unretained(this)));
  }
  return fullscreen_control_popup_.get();
}

bool FullscreenControlHost::IsPopupCreated() const {
  return fullscreen_control_popup_.get() != nullptr;
}

bool FullscreenControlHost::IsAnimating() const {
  return IsPopupCreated() && fullscreen_control_popup_->IsAnimating();
}

void FullscreenControlHost::ShowForInputEntryMethod(
    InputEntryMethod input_entry_method) {
  input_entry_method_ = input_entry_method;
  auto* bubble = exclusive_access_context_->GetExclusiveAccessBubble();
  if (bubble)
    bubble->HideImmediately();
  GetPopup()->Show(bubble_views_context_->GetClientAreaBoundsInScreen());

  // Exit cooldown mode in case the exit UI is triggered by a different method.
  in_mouse_cooldown_mode_ = false;
}

void FullscreenControlHost::OnVisibilityChanged() {
  if (!IsVisible()) {
    input_entry_method_ = InputEntryMethod::NOT_ACTIVE;
    key_press_delay_timer_.Stop();
  } else if (input_entry_method_ == InputEntryMethod::MOUSE) {
    StartPopupTimeout(InputEntryMethod::MOUSE);
  }

  if (on_popup_visibility_changed_)
    std::move(on_popup_visibility_changed_).Run();
}

void FullscreenControlHost::StartPopupTimeout(
    InputEntryMethod expected_input_method) {
  popup_timeout_timer_.Start(
      FROM_HERE, kPopupTimeout,
      base::BindRepeating(&FullscreenControlHost::OnPopupTimeout,
                          base::Unretained(this), expected_input_method));
}

void FullscreenControlHost::OnPopupTimeout(
    InputEntryMethod expected_input_method) {
  if (IsVisible() && !IsAnimating() &&
      input_entry_method_ == expected_input_method) {
    if (input_entry_method_ == InputEntryMethod::MOUSE)
      in_mouse_cooldown_mode_ = true;
    Hide(true);
  }
}

bool FullscreenControlHost::IsExitUiNeeded() {
  return exclusive_access_context_->IsFullscreen() &&
         exclusive_access_context_->ShouldHideUIForFullscreen();
}

float FullscreenControlHost::CalculateCursorBufferHeight() const {
  float control_bottom =
      FullscreenControlPopup::GetButtonBottomOffset() +
      bubble_views_context_->GetClientAreaBoundsInScreen().y();
  DCHECK_GT(control_bottom, 0);
  return control_bottom * kExitHeightScaleFactor;
}
