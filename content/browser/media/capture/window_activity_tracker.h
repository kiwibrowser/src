// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_WINDOW_ACTIVITY_TRACKER_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_WINDOW_ACTIVITY_TRACKER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "ui/gfx/native_widget_types.h"

namespace content {

// WindowActivityTracker is an interface that can be implememented to report
// whether the user is actively interacting with UI.
class CONTENT_EXPORT WindowActivityTracker {
 public:
  static std::unique_ptr<WindowActivityTracker> Create(gfx::NativeView view);

  WindowActivityTracker();
  virtual ~WindowActivityTracker();

  // Returns true if UI interaction is active.
  bool IsUiInteractionActive() const;

  // Reports on every mouse movement activity on the window.
  void RegisterMouseInteractionObserver(const base::Closure& observer);

  // Resets any previous UI activity tracked.
  void Reset();

  // Returns a weak pointer.
  virtual base::WeakPtr<WindowActivityTracker> GetWeakPtr() = 0;

 protected:
  void OnMouseActivity();

 private:
  // The last time a UI event was detected.
  base::TimeTicks last_time_ui_event_detected_;

  // Runs on any mouse interaction from user.
  base::Closure mouse_interaction_observer_;

  // The number of UI events detected so far. In case of continuous events
  // such as mouse movement, a single continuous movement is treated
  // as one event.
  int ui_events_count_;

  DISALLOW_COPY_AND_ASSIGN(WindowActivityTracker);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_WINDOW_ACTIVITY_TRACKER_H_
