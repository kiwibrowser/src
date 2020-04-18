// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/scoped_visibility_tracker.h"

#include <utility>

#include "base/time/tick_clock.h"

ScopedVisibilityTracker::ScopedVisibilityTracker(
    const base::TickClock* tick_clock,
    bool is_shown)
    : tick_clock_(tick_clock) {
  DCHECK(tick_clock_);
  if (is_shown)
    OnShown();
}

ScopedVisibilityTracker::~ScopedVisibilityTracker() {}

void ScopedVisibilityTracker::OnShown() {
  Update(true /* in_foreground */);
}

void ScopedVisibilityTracker::OnHidden() {
  Update(false /* in_foreground */);
}

base::TimeDelta ScopedVisibilityTracker::GetForegroundDuration() {
  Update(currently_in_foreground_);
  return foreground_duration_;
}

void ScopedVisibilityTracker::Update(bool in_foreground) {
  base::TimeTicks now = tick_clock_->NowTicks();
  if (currently_in_foreground_)
    foreground_duration_ += now - last_time_shown_;

  if (in_foreground)
    last_time_shown_ = now;

  currently_in_foreground_ = in_foreground;
}
