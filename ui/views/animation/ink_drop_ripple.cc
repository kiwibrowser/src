// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/animation/ink_drop_ripple.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "ui/base/ui_base_switches.h"
#include "ui/compositor/callback_layer_animation_observer.h"
#include "ui/compositor/layer.h"

namespace views {

const double InkDropRipple::kSlowAnimationDurationFactor = 3.0;

bool InkDropRipple::UseFastAnimations() {
  static bool fast =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          (::switches::kMaterialDesignInkDropAnimationSpeed)) !=
      ::switches::kMaterialDesignInkDropAnimationSpeedSlow;
  return fast;
}

const float InkDropRipple::kHiddenOpacity = 0.f;

InkDropRipple::InkDropRipple()
    : target_ink_drop_state_(InkDropState::HIDDEN), observer_(nullptr) {}

InkDropRipple::~InkDropRipple() {}

void InkDropRipple::HostSizeChanged(const gfx::Size& new_size) {}

void InkDropRipple::AnimateToState(InkDropState ink_drop_state) {
  // Does not return early if |target_ink_drop_state_| == |ink_drop_state| for
  // two reasons.
  // 1. The attached observers must be notified of all animations started and
  // ended.
  // 2. Not all state transitions is are valid, especially no-op transitions,
  // and these invalid transitions will be logged as warnings in
  // AnimateStateChange().

  // |animation_observer| will be deleted when AnimationEndedCallback() returns
  // true.
  // TODO(bruthig): Implement a safer ownership model for the
  // |animation_observer|.
  ui::CallbackLayerAnimationObserver* animation_observer =
      new ui::CallbackLayerAnimationObserver(
          base::Bind(&InkDropRipple::AnimationStartedCallback,
                     base::Unretained(this), ink_drop_state),
          base::Bind(&InkDropRipple::AnimationEndedCallback,
                     base::Unretained(this), ink_drop_state));

  InkDropState old_ink_drop_state = target_ink_drop_state_;
  // Assign to |target_ink_drop_state_| before calling AnimateStateChange() so
  // that any observers notified as a side effect of the AnimateStateChange()
  // will get the target InkDropState when calling GetInkDropState().
  target_ink_drop_state_ = ink_drop_state;

  if (old_ink_drop_state == InkDropState::HIDDEN &&
      target_ink_drop_state_ != InkDropState::HIDDEN) {
    GetRootLayer()->SetVisible(true);
  }

  AnimateStateChange(old_ink_drop_state, target_ink_drop_state_,
                     animation_observer);
  animation_observer->SetActive();
  // |this| may be deleted! |animation_observer| might synchronously call
  // AnimationEndedCallback which can delete |this|.
}

void InkDropRipple::SnapToActivated() {
  AbortAllAnimations();
  // |animation_observer| will be deleted when AnimationEndedCallback() returns
  // true.
  // TODO(bruthig): Implement a safer ownership model for the
  // |animation_observer|.
  ui::CallbackLayerAnimationObserver* animation_observer =
      new ui::CallbackLayerAnimationObserver(
          base::Bind(&InkDropRipple::AnimationStartedCallback,
                     base::Unretained(this), InkDropState::ACTIVATED),
          base::Bind(&InkDropRipple::AnimationEndedCallback,
                     base::Unretained(this), InkDropState::ACTIVATED));
  GetRootLayer()->SetVisible(true);
  target_ink_drop_state_ = InkDropState::ACTIVATED;
  animation_observer->SetActive();
}

bool InkDropRipple::IsVisible() {
  return GetRootLayer()->visible();
}

void InkDropRipple::SnapToHidden() {
  AbortAllAnimations();
  SetStateToHidden();
  target_ink_drop_state_ = InkDropState::HIDDEN;
}

test::InkDropRippleTestApi* InkDropRipple::GetTestApi() {
  return nullptr;
}

void InkDropRipple::AnimationStartedCallback(
    InkDropState ink_drop_state,
    const ui::CallbackLayerAnimationObserver& observer) {
  if (observer_)
    observer_->AnimationStarted(ink_drop_state);
}

bool InkDropRipple::AnimationEndedCallback(
    InkDropState ink_drop_state,
    const ui::CallbackLayerAnimationObserver& observer) {
  if (ink_drop_state == InkDropState::HIDDEN)
    SetStateToHidden();
  if (observer_)
    observer_->AnimationEnded(ink_drop_state,
                              observer.aborted_count()
                                  ? InkDropAnimationEndedReason::PRE_EMPTED
                                  : InkDropAnimationEndedReason::SUCCESS);
  // |this| may be deleted!
  return true;
}

}  // namespace views
