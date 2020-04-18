// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_UI_UPDATER_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_UI_UPDATER_H_

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_observer.h"

@protocol FullscreenUIElement;

// Observer that updates UI elements for FullscreenController.
class FullscreenUIUpdater : public FullscreenControllerObserver {
 public:
  // Contructor for an observer that updates |ui_element|.  |ui_element| is not
  // retained.
  explicit FullscreenUIUpdater(id<FullscreenUIElement> ui_element);

 private:
  // FullscreenControllerObserver:
  void FullscreenProgressUpdated(FullscreenController* controller,
                                 CGFloat progress) override;
  void FullscreenEnabledStateChanged(FullscreenController* controller,
                                     bool enabled) override;
  void FullscreenScrollEventEnded(FullscreenController* controller,
                                  FullscreenAnimator* animator) override;
  void FullscreenWillScrollToTop(FullscreenController* controller,
                                 FullscreenAnimator* animator) override;
  void FullscreenWillEnterForeground(FullscreenController* controller,
                                     FullscreenAnimator* animator) override;
  void FullscreenModelWasReset(FullscreenController* controller,
                               FullscreenAnimator* animator) override;

  // The UI element being updated by this observer.
  __weak id<FullscreenUIElement> ui_element_;
};

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_UI_UPDATER_H_
