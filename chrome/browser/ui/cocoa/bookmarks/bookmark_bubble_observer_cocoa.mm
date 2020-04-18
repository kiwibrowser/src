// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bubble_observer_cocoa.h"

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"

BookmarkBubbleObserverCocoa::BookmarkBubbleObserverCocoa(
    BrowserWindowController* controller)
    : controller_(controller), lockOwner_([[NSObject alloc] init]) {}

BookmarkBubbleObserverCocoa::~BookmarkBubbleObserverCocoa() {}

void BookmarkBubbleObserverCocoa::OnBookmarkBubbleShown(
    const bookmarks::BookmarkNode* node) {
  [controller_ lockToolbarVisibilityForOwner:lockOwner_ withAnimation:NO];
  [[controller_ bookmarkBarController] startPulsingBookmarkNode:node];
}

void BookmarkBubbleObserverCocoa::OnBookmarkBubbleHidden() {
  [controller_ releaseToolbarVisibilityForOwner:lockOwner_ withAnimation:YES];
  [[controller_ bookmarkBarController] stopPulsingBookmarkNode];
  [controller_ bookmarkBubbleClosed];
}
