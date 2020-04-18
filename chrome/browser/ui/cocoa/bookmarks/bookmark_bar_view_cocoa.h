// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_VIEW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_VIEW_COCOA_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"

@class BookmarkBarController;

namespace bookmarks {
const CGFloat kInitialNoItemTextFieldXOrigin = 5;
}

// A simple custom NSView for the bookmark bar used to prevent clicking and
// dragging from moving the browser window.
@interface BookmarkBarView : NSView {
 @private
  BOOL dropIndicatorShown_;
  CGFloat dropIndicatorPosition_;  // x position

  BookmarkBarController* controller_;
  base::scoped_nsobject<NSTextField> noItemTextField_;
  base::scoped_nsobject<NSButton> importBookmarksButton_;
}
- (NSTextField*)noItemTextField;
- (NSButton*)importBookmarksButton;

- (instancetype)initWithController:(BookmarkBarController*)controller
                             frame:(NSRect)frame;
- (void)registerForNotificationsAndDraggedTypes;

// Exposed so the controller can nil itself out.
@property(nonatomic, assign) BookmarkBarController* controller;
@end

@interface BookmarkBarView()  // TestingOrInternalAPI
@property(nonatomic, readonly) BOOL dropIndicatorShown;
@property(nonatomic, readonly) CGFloat dropIndicatorPosition;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_VIEW_COCOA_H_
