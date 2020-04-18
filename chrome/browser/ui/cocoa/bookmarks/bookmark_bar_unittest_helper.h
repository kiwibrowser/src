// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_UNITTEST_HELPER_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_UNITTEST_HELPER_H_

#import <Foundation/Foundation.h>

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"

@interface BookmarkBarController (BookmarkBarUnitTestHelper)

// Return the bookmark button from this bar controller with the given
// |title|, otherwise nil. This does not recurse into folders.
- (BookmarkButton*)buttonWithTitleEqualTo:(NSString*)title;

@end


@interface BookmarkBarFolderController (BookmarkBarUnitTestHelper)

// Return the bookmark button from this folder controller with the given
// |title|, otherwise nil. This does not recurse into subfolders.
- (BookmarkButton*)buttonWithTitleEqualTo:(NSString*)title;

@end


@interface BookmarkButton (BookmarkBarUnitTestHelper)

// Return the center of the button in the base coordinate system of the
// containing window. Useful for simulating mouse clicks or drags.
- (NSPoint)center;

// Return the top of the button in the base coordinate system of the
// containing window.
- (NSPoint)top;

// Return the bottom of the button in the base coordinate system of the
// containing window.
- (NSPoint)bottom;

// Return the center-left point of the button in the base coordinate system
// of the containing window.
- (NSPoint)left;

// Return the center-right point of the button in the base coordinate system
// of the containing window.
- (NSPoint)right;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_UNITTEST_HELPER_H_
