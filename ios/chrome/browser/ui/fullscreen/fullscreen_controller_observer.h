// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_OBSERVER_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_OBSERVER_H_

#include <CoreGraphics/CoreGraphics.h>

#include "base/macros.h"

class FullscreenController;
@class FullscreenAnimator;

// Interface for listening to fullscreen state.
class FullscreenControllerObserver {
 public:
  FullscreenControllerObserver() = default;
  virtual ~FullscreenControllerObserver() = default;

  // Invoked after a scrolling event has caused |controller| to calculate
  // |progress|.  A |progress| value of 1.0 denotes that the toolbar should be
  // completely visible, while a |progress| value of 0.0 denotes that the
  // toolbar should be competely hidden.
  virtual void FullscreenProgressUpdated(FullscreenController* controller,
                                         CGFloat progress) {}

  // Invoked with |controller| is enabled or disabled.
  virtual void FullscreenEnabledStateChanged(FullscreenController* controller,
                                             bool enabled) {}

  // Invoked when a scroll event being observed by |controller| has ended.
  // Observers can add animations to |animator|.
  virtual void FullscreenScrollEventEnded(FullscreenController* controller,
                                          FullscreenAnimator* animator) {}

  // Invoked to scroll the main content view to the top.  FullscreenUIElements
  // are expected add animations that scroll the content to the top and fully
  // show the toolbar.
  virtual void FullscreenWillScrollToTop(FullscreenController* controller,
                                         FullscreenAnimator* animator) {}

  // Invoked when the application is about to enter the foreground.
  // FullscreenUIElements are expected to add animations to |animator| to show
  // the toolbar.
  virtual void FullscreenWillEnterForeground(FullscreenController* controller,
                                             FullscreenAnimator* animator) {}

  // Invoked when FullscreenController::ResetModel() is called.
  // FullscreenUIElements are expected to add animations to |animator| to show
  // the toolbar.
  virtual void FullscreenModelWasReset(FullscreenController* controller,
                                       FullscreenAnimator* animator) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FullscreenControllerObserver);
};

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_CONTROLLER_OBSERVER_H_
