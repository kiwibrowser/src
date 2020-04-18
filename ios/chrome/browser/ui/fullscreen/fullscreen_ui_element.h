// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_UI_ELEMENT_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_UI_ELEMENT_H_

#import <Foundation/Foundation.h>

@class FullscreenAnimator;

// UI elements that need to react to Fullscreen events should conform to this
// protocol to react to changes in Fullscreen state.
@protocol FullscreenUIElement<NSObject>

// Tells the UI to update its state for |progress|.  A fullscreen |progress|
// value denotes that the toolbar should be completely visible, and a |progress|
// value of 0.0 denotes that the toolbar should be completely hidden.
- (void)updateForFullscreenProgress:(CGFloat)progress;

// Tells the UI that fullscreen is enabled or disabled.  When disabled, the UI
// should immediately be updated to the state corresponding with a progress
// value of 1.0.
- (void)updateForFullscreenEnabled:(BOOL)enabled;

// Called when a fullscreen scroll event has finished.  UI elements that react
// to fullscreen events can configure |animator| with animations.
- (void)finishFullscreenScrollWithAnimator:(FullscreenAnimator*)animator;

// Called when a scroll-to-top animation is triggered.  UI elements that react
// to fullscreen events can configure |animator| with animations.
- (void)scrollFullscreenToTopWithAnimator:(FullscreenAnimator*)animator;

// Called to show the toolbar.  This occurs when the app is foregrounded, or
// when promped by FullscreenController::ResetModel().  UI elements that react
// to fullscreen events can configure |animator| with aniamtions.
- (void)showToolbarWithAnimator:(FullscreenAnimator*)animator;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_FULLSCREEN_FULLSCREEN_UI_ELEMENT_H_
