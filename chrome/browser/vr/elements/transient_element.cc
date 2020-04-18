// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/transient_element.h"
#include "base/callback_helpers.h"

namespace vr {

TransientElement::TransientElement(const base::TimeDelta& timeout)
    : timeout_(timeout) {
  SetVisibleImmediately(false);
}

TransientElement::~TransientElement() {}

void TransientElement::SetVisible(bool visible) {
  bool will_be_visible = GetTargetOpacity() == opacity_when_visible();
  // We're already at the desired visibility, no-op.
  if (visible == will_be_visible)
    return;

  if (visible)
    Reset();

  super::SetVisible(visible);
}

void TransientElement::SetVisibleImmediately(bool visible) {
  bool will_be_visible = GetTargetOpacity() == opacity_when_visible();
  if (!will_be_visible && visible)
    Reset();

  super::SetVisibleImmediately(visible);
}

void TransientElement::RefreshVisible() {
  // Do nothing if we're not going to be visible.
  if (GetTargetOpacity() != opacity_when_visible())
    return;
  Reset();
}

void TransientElement::Reset() {
  set_visible_time_ = base::TimeTicks();
}

SimpleTransientElement::SimpleTransientElement(const base::TimeDelta& timeout)
    : super(timeout) {}

SimpleTransientElement::~SimpleTransientElement() {}

bool SimpleTransientElement::OnBeginFrame(const gfx::Transform& head_pose) {
  // Do nothing if we're not going to be visible.
  if (GetTargetOpacity() != opacity_when_visible())
    return false;

  // SetVisible may have been called during initialization which means that the
  // last frame time would be zero.
  if (set_visible_time_.is_null() && opacity() > 0.0f)
    set_visible_time_ = last_frame_time();

  base::TimeDelta duration = last_frame_time() - set_visible_time_;

  if (!set_visible_time_.is_null() && duration >= timeout_) {
    super::SetVisible(false);
    return true;
  }
  return false;
}

ShowUntilSignalTransientElement::ShowUntilSignalTransientElement(
    const base::TimeDelta& min_duration,
    const base::TimeDelta& timeout,
    OnMinDurationCallback min_duration_callback,
    OnHideCallback hide_callback)
    : super(timeout),
      min_duration_(min_duration),
      min_duration_callback_(min_duration_callback),
      hide_callback_(hide_callback) {
  SetVisibleImmediately(false);
}

ShowUntilSignalTransientElement::~ShowUntilSignalTransientElement() {}

bool ShowUntilSignalTransientElement::OnBeginFrame(
    const gfx::Transform& head_pose) {
  // Do nothing if we're not going to be visible.
  if (GetTargetOpacity() != opacity_when_visible())
    return false;

  // SetVisible may have been called during initialization which means that the
  // last frame time would be zero.
  if (set_visible_time_.is_null() && opacity() > 0.0f)
    set_visible_time_ = last_frame_time();

  bool set_invisible = false;

  base::TimeDelta duration = last_frame_time() - set_visible_time_;
  if (!set_visible_time_.is_null() && !min_duration_callback_called_ &&
      duration >= min_duration_) {
    min_duration_callback_.Run();
    min_duration_callback_called_ = true;
  }

  if (!set_visible_time_.is_null() && duration > timeout_) {
    hide_callback_.Run(TransientElementHideReason::kTimeout);
    set_invisible = true;
  } else if (!set_visible_time_.is_null() && duration >= min_duration_ &&
             signaled_) {
    hide_callback_.Run(TransientElementHideReason::kSignal);
    set_invisible = true;
  }
  if (set_invisible) {
    super::SetVisible(false);
    return true;
  }
  return false;
}

void ShowUntilSignalTransientElement::Signal(bool value) {
  signaled_ = value;
}

void ShowUntilSignalTransientElement::Reset() {
  min_duration_callback_called_ = false;
  super::Reset();
}

}  // namespace vr
