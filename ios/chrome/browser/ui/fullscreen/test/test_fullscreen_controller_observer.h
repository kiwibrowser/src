// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_FULLSCREEN_TEST_TEST_FULLSCREEN_CONTROLLER_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_FULLSCREEN_TEST_TEST_FULLSCREEN_CONTROLLER_OBSERVER_H_

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_observer.h"

// Test version of FullscreenControllerObserver.
class TestFullscreenControllerObserver : public FullscreenControllerObserver {
 public:
  CGFloat progress() const { return progress_; }
  bool enabled() const { return enabled_; }
  FullscreenAnimator* animator() const { return animator_; }

 private:
  // FullscreenControllerObserver:
  void FullscreenProgressUpdated(FullscreenController* controller,
                                 CGFloat progress) override;
  void FullscreenEnabledStateChanged(FullscreenController* controller,
                                     bool enabled) override;
  void FullscreenScrollEventEnded(FullscreenController* controller,
                                  FullscreenAnimator* animator) override;
  void FullscreenWillEnterForeground(FullscreenController* controller,
                                     FullscreenAnimator* animator) override;
  void FullscreenModelWasReset(FullscreenController* controller,
                               FullscreenAnimator* aniamtor) override;

  CGFloat progress_ = 0.0;
  bool enabled_ = true;
  __weak FullscreenAnimator* animator_ = nil;
};

#endif  // IOS_CHROME_BROWSER_UI_FULLSCREEN_TEST_TEST_FULLSCREEN_CONTROLLER_OBSERVER_H_
