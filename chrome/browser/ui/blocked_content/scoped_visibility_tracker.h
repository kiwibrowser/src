// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_SCOPED_VISIBILITY_TRACKER_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_SCOPED_VISIBILITY_TRACKER_H_

#include <memory>

#include "base/macros.h"
#include "base/time/time.h"

namespace base {
class TickClock;
}  // namespace base

// This class tracks the total time it is visible, based on receiving
// OnShown/OnHidden notifications, which are logically idempotent.
class ScopedVisibilityTracker {
 public:
  // |tick_clock| must outlive this object.
  ScopedVisibilityTracker(const base::TickClock* tick_clock, bool is_shown);
  ~ScopedVisibilityTracker();

  void OnShown();
  void OnHidden();

  base::TimeDelta GetForegroundDuration();

 private:
  void Update(bool in_foreground);

  const base::TickClock* tick_clock_;

  base::TimeTicks last_time_shown_;
  base::TimeDelta foreground_duration_;
  bool currently_in_foreground_ = false;

  DISALLOW_COPY_AND_ASSIGN(ScopedVisibilityTracker);
};

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_SCOPED_VISIBILITY_TRACKER_H_
