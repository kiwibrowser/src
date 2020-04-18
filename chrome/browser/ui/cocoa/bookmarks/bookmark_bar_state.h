// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_STATE_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_STATE_H_

#include <CoreGraphics/CoreGraphics.h>

#include "chrome/browser/ui/bookmarks/bookmark_bar.h"

// The interface for controllers (etc.) which can give information about the
// bookmark bar's state.
@protocol BookmarkBarState

// Returns YES if the bookmark bar is currently visible (as a normal toolbar or
// as a detached bar on the NTP), NO otherwise.
- (BOOL)isVisible;

// Returns YES if an animation is currently running, NO otherwise.
- (BOOL)isAnimationRunning;

// Returns YES if the bookmark bar is in the given state and not in an
// animation, NO otherwise.
- (BOOL)isInState:(BookmarkBar::State)state;

// Returns YES if the bookmark bar is animating from the given state (to any
// other state), NO otherwise.
- (BOOL)isAnimatingToState:(BookmarkBar::State)state;

// Returns YES if the bookmark bar is animating to the given state (from any
// other state), NO otherwise.
- (BOOL)isAnimatingFromState:(BookmarkBar::State)state;

// Returns YES if the bookmark bar is animating from the first given state to
// the second given state, NO otherwise.
- (BOOL)isAnimatingFromState:(BookmarkBar::State)fromState
                     toState:(BookmarkBar::State)toState;

// Returns YES if the bookmark bar is animating between the two given states (in
// either direction), NO otherwise.
- (BOOL)isAnimatingBetweenState:(BookmarkBar::State)fromState
                       andState:(BookmarkBar::State)toState;

// Returns how morphed into the detached bubble the bookmark bar should be (1 =
// completely detached, 0 = normal).
- (CGFloat)detachedMorphProgress;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_STATE_H_
