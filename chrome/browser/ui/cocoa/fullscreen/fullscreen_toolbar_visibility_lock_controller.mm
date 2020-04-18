// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_visibility_lock_controller.h"

#include "base/mac/scoped_nsobject.h"

#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_animation_controller.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_controller.h"

@interface FullscreenToolbarVisibilityLockController () {
  // Stores the objects that are locking the toolbar visibility.
  base::scoped_nsobject<NSMutableSet> visibilityLocks_;

  // Our owner.
  FullscreenToolbarController* owner_;  // weak

  // The object managing the fullscreen toolbar's animations.
  FullscreenToolbarAnimationController* animationController_;  // weak
}

@end

@implementation FullscreenToolbarVisibilityLockController

- (instancetype)
initWithFullscreenToolbarController:(FullscreenToolbarController*)owner
                animationController:
                    (FullscreenToolbarAnimationController*)animationController {
  if ((self = [super init])) {
    animationController_ = animationController;
    owner_ = owner;

    // Create the toolbar visibility lock set; 10 is arbitrary, but should
    // hopefully be big enough to hold all locks that'll ever be needed.
    visibilityLocks_.reset([[NSMutableSet setWithCapacity:10] retain]);
  }

  return self;
}

- (BOOL)isToolbarVisibilityLocked {
  return [visibilityLocks_ count];
}

- (BOOL)isToolbarVisibilityLockedForOwner:(id)owner {
  return [visibilityLocks_ containsObject:owner];
}

- (void)lockToolbarVisibilityForOwner:(id)owner withAnimation:(BOOL)animate {
  if ([self isToolbarVisibilityLockedForOwner:owner])
    return;

  [visibilityLocks_ addObject:owner];

  if (animate)
    animationController_->AnimateToolbarIn();
  else
    [owner_ layoutToolbar];
}

- (void)releaseToolbarVisibilityForOwner:(id)owner withAnimation:(BOOL)animate {
  if (![self isToolbarVisibilityLockedForOwner:owner])
    return;

  [visibilityLocks_ removeObject:owner];

  if (animate)
    animationController_->AnimateToolbarOutIfPossible();
  else
    [owner_ layoutToolbar];
}

@end
