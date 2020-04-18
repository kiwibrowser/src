// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TOOLBAR_APP_TOOLBAR_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_TOOLBAR_APP_TOOLBAR_BUTTON_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#import "chrome/browser/ui/cocoa/menu_button.h"
#include "chrome/browser/ui/toolbar/app_menu_icon_controller.h"

class AnimatedIcon;

// Button for the app toolbar button.
@interface AppToolbarButton : MenuButton {
 @private
  AppMenuIconController::Severity severity_;
  AppMenuIconController::IconType type_;

  // Used for animating and drawing the icon.
  std::unique_ptr<AnimatedIcon> animatedIcon_;

  // Value of the kAnimatedAppMenuIcon feature's "HasDelay" param.
  BOOL isDelayEnabled_;

  // Used to delay the animation. Not used if |isDelayEnabled_| is false.
  // We own the timer and release it when it's completed.
  NSTimer* animationDelayTimer_;

  // Set true if the timer should be disabled for testing. If it's true,
  // the timer will still be created but the duration will be set to 0.
  BOOL disableTimerForTesting_;
}

- (void)setSeverity:(AppMenuIconController::Severity)severity
           iconType:(AppMenuIconController::IconType)iconType
      shouldAnimate:(BOOL)shouldAnimate;

// Animates the icon if possible. If |isDelayEnabled_| and |delay|
// is true, then set |animationDelayTimer_| if it's not already set.
- (void)animateIfPossibleWithDelay:(BOOL)delay;

@end

@interface AppToolbarButton (ExposedForTesting)

// Returns |animatedIcon_|.get().
- (AnimatedIcon*)animatedIcon;

// Sets |animatedIcon_|.
- (void)setAnimatedIcon:(AnimatedIcon*)icon;

// Returns |animationDelayTimer_|.
- (NSTimer*)animationDelayTimer;

// Sets |disableTimerForTesting_|.
- (void)setDisableTimerForTest:(BOOL)disable;
@end

#endif  // CHROME_BROWSER_UI_COCOA_TOOLBAR_APP_TOOLBAR_BUTTON_H_
