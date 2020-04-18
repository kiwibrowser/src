// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SCHEDULER_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SCHEDULER_OBSERVER_H_

#include "base/macros.h"

class OverlayScheduler;
namespace web {
class WebState;
}

// Interface for listening to OverlayService events.
class OverlaySchedulerObserver {
 public:
  OverlaySchedulerObserver() = default;
  virtual ~OverlaySchedulerObserver() = default;

  // Tells the observer that |scheduler| is about to display an overlay.  If
  // |web_state| is non-null, then this is a WebState-specific overlay, and the
  // WebState's content view is expected to be visible after this callback.
  virtual void OverlaySchedulerWillShowOverlay(OverlayScheduler* scheduler,
                                               web::WebState* web_state) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(OverlaySchedulerObserver);
};

#endif  // IOS_CHROME_BROWSER_UI_OVERLAYS_OVERLAY_SCHEDULER_OBSERVER_H_
