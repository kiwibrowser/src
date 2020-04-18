// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FULLSCREEN_FULLSCREEN_TOOLBAR_VISIBILITY_LOCK_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_FULLSCREEN_FULLSCREEN_TOOLBAR_VISIBILITY_LOCK_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

class FullscreenToolbarAnimationController;
@class FullscreenToolbarController;

// Various UI elements/events may want to ensure that the toolbar is visible in
// fullscreen mode. Whenever an object requires toolbar visibility, it locks
// it. When it no longer requires it, it releases it. This class manages the
// toolbar visibility locks.
@interface FullscreenToolbarVisibilityLockController : NSObject

// The designated initializer.
- (instancetype)
initWithFullscreenToolbarController:(FullscreenToolbarController*)controller
                animationController:
                    (FullscreenToolbarAnimationController*)animationController;

// Returns true if the toolbar visibility is locked.
- (BOOL)isToolbarVisibilityLocked;

// Returns true if the toolbar visibility is locked by |owner|.
- (BOOL)isToolbarVisibilityLockedForOwner:(id)owner;

// Methods for locking and releasing the toolbar visibility. If |animate| is
// true, the toolbar will animate in/out.
- (void)lockToolbarVisibilityForOwner:(id)owner withAnimation:(BOOL)animate;
- (void)releaseToolbarVisibilityForOwner:(id)owner withAnimation:(BOOL)animate;

@end

#endif  // CHROME_BROWSER_UI_COCOA_FULLSCREEN_FULLSCREEN_TOOLBAR_VISIBILITY_LOCK_CONTROLLER_H_
