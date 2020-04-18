// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_TOOLBAR_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_TOOLBAR_VIEW_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/animatable_view.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_state.h"

class Profile;

// An interface to allow mocking of a BookmarkBarController by the
// BookmarkBarToolbarView.
@protocol BookmarkBarToolbarViewController <BookmarkBarState>
// Displaying the bookmark toolbar background in bubble (floating) mode requires
// the size of the currently selected tab to properly calculate where the
// background image is joined.
- (int)currentTabContentsHeight;

// Current profile.
- (Profile*)profile;

@end

// The BookmarkBarToolbarView is responsible for drawing the background of the
// BookmarkBar's toolbar in either of its two display modes - permanently
// attached (slimline with a stroke at the bottom edge) or New Tab Page style
// (padded with a round rect border and the New Tab Page theme behind).
@interface BookmarkBarToolbarView : AnimatableView {
 @private
  // The controller which tells us how we should be drawing (as normal or as a
  // floating bar). Weak reference.
  id<BookmarkBarToolbarViewController> controller_;
}

// Exposed so that the controller can nil itself out
- (void)setController:(id<BookmarkBarToolbarViewController>)controller;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_TOOLBAR_VIEW_H_
