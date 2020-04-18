// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/autoclick/common/autoclick_controller_common.h"

#include "ash/autoclick/common/autoclick_controller_common_delegate.h"
#include "ui/aura/window.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ash {

namespace {

// The threshold of mouse movement measured in DIP that will
// initiate a new autoclick.
const int kMovementThreshold = 20;

bool IsModifierKey(const ui::KeyboardCode key_code) {
  return key_code == ui::VKEY_SHIFT || key_code == ui::VKEY_LSHIFT ||
         key_code == ui::VKEY_CONTROL || key_code == ui::VKEY_LCONTROL ||
         key_code == ui::VKEY_RCONTROL || key_code == ui::VKEY_MENU ||
         key_code == ui::VKEY_LMENU || key_code == ui::VKEY_RMENU;
}

}  // namespace

AutoclickControllerCommon::AutoclickControllerCommon(
    base::TimeDelta delay,
    AutoclickControllerCommonDelegate* delegate)
    : delay_(delay),
      mouse_event_flags_(ui::EF_NONE),
      delegate_(delegate),
      widget_(nullptr),
      anchor_location_(-kMovementThreshold, -kMovementThreshold),
      autoclick_ring_handler_(new AutoclickRingHandler()) {
  InitClickTimer();
}

AutoclickControllerCommon::~AutoclickControllerCommon() = default;

void AutoclickControllerCommon::HandleMouseEvent(const ui::MouseEvent& event) {
  gfx::Point mouse_location = event.location();
  if (event.type() == ui::ET_MOUSE_MOVED &&
      !(event.flags() & ui::EF_IS_SYNTHESIZED)) {
    mouse_event_flags_ = event.flags();

    // TODO(riajiang): Make getting screen location work for mus as well.
    // Also switch to take in a PointerEvent instead of a MouseEvent after
    // this is done. (crbug.com/608547)
    if (event.target() != nullptr) {
      ::wm::ConvertPointToScreen(static_cast<aura::Window*>(event.target()),
                                 &mouse_location);
    }
    UpdateRingWidget(mouse_location);

    // The distance between the mouse location and the anchor location
    // must exceed a certain threshold to initiate a new autoclick countdown.
    // This ensures that mouse jitter caused by poor motor control does not
    // 1. initiate an unwanted autoclick from rest
    // 2. prevent the autoclick from ever occurring when the mouse
    //    arrives at the target.
    gfx::Vector2d delta = mouse_location - anchor_location_;
    if (delta.LengthSquared() >= kMovementThreshold * kMovementThreshold) {
      anchor_location_ = mouse_location;
      autoclick_timer_->Reset();
      autoclick_ring_handler_->StartGesture(delay_, anchor_location_, widget_);
    } else if (autoclick_timer_->IsRunning()) {
      autoclick_ring_handler_->SetGestureCenter(mouse_location, widget_);
    }
  } else if (event.type() == ui::ET_MOUSE_PRESSED) {
    CancelAutoclick();
  } else if (event.type() == ui::ET_MOUSEWHEEL &&
             autoclick_timer_->IsRunning()) {
    autoclick_timer_->Reset();
    UpdateRingWidget(mouse_location);
    autoclick_ring_handler_->StartGesture(delay_, anchor_location_, widget_);
  }
}

void AutoclickControllerCommon::HandleKeyEvent(const ui::KeyEvent& event) {
  int modifier_mask = ui::EF_SHIFT_DOWN | ui::EF_CONTROL_DOWN |
                      ui::EF_ALT_DOWN | ui::EF_COMMAND_DOWN |
                      ui::EF_IS_EXTENDED_KEY;
  int new_modifiers = event.flags() & modifier_mask;
  mouse_event_flags_ = (mouse_event_flags_ & ~modifier_mask) | new_modifiers;

  if (!IsModifierKey(event.key_code()))
    CancelAutoclick();
}

void AutoclickControllerCommon::SetAutoclickDelay(const base::TimeDelta delay) {
  delay_ = delay;
  InitClickTimer();
}

void AutoclickControllerCommon::CancelAutoclick() {
  autoclick_timer_->Stop();
  autoclick_ring_handler_->StopGesture();
  delegate_->OnAutoclickCanceled();
}

void AutoclickControllerCommon::InitClickTimer() {
  autoclick_timer_.reset(new base::Timer(
      FROM_HERE, delay_, base::Bind(&AutoclickControllerCommon::DoAutoclick,
                                    base::Unretained(this)),
      false));
}

void AutoclickControllerCommon::DoAutoclick() {
  gfx::Point screen_location =
      display::Screen::GetScreen()->GetCursorScreenPoint();
  anchor_location_ = screen_location;
  delegate_->DoAutoclick(screen_location, mouse_event_flags_);
}

void AutoclickControllerCommon::UpdateRingWidget(
    const gfx::Point& mouse_location) {
  if (!widget_) {
    widget_ = delegate_->CreateAutoclickRingWidget(mouse_location);
  } else {
    delegate_->UpdateAutoclickRingWidget(widget_, mouse_location);
  }
}

}  // namespace ash
