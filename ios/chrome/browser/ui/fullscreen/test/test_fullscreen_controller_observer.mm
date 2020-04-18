// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/test/test_fullscreen_controller_observer.h"

#import "ios/chrome/browser/ui/fullscreen/fullscreen_animator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void TestFullscreenControllerObserver::FullscreenProgressUpdated(
    FullscreenController* controller,
    CGFloat progress) {
  progress_ = progress;
}

void TestFullscreenControllerObserver::FullscreenEnabledStateChanged(
    FullscreenController* controller,
    bool enabled) {
  enabled_ = enabled;
}

void TestFullscreenControllerObserver::FullscreenScrollEventEnded(
    FullscreenController* controller,
    FullscreenAnimator* animator) {
  animator_ = animator;
}

void TestFullscreenControllerObserver::FullscreenWillEnterForeground(
    FullscreenController* controller,
    FullscreenAnimator* animator) {
  animator_ = animator;
}

void TestFullscreenControllerObserver::FullscreenModelWasReset(
    FullscreenController* controller,
    FullscreenAnimator* animator) {
  animator_ = animator;
}
