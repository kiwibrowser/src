// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_WINDOW_H_

#import <Cocoa/Cocoa.h>

// Window for a bookmark folder "menu".  This menu pops up when you
// click on a bookmark button that represents a folder of bookmarks.
// This window is borderless but has a shadow, at least initially.
// Once the down scroll arrow is shown, the shadow is turned off and
// a secondary window is added which is sized to match the visible
// area of the menu and which provides the shadow.
@interface BookmarkBarFolderWindow : NSWindow
@end

// Content view for the above window.  "Stock" other than the drawing
// of rounded corners.  Only used in the nib.
@interface BookmarkBarFolderWindowContentView : NSView
// Returns the folder window's background color (Material Design only).
+ (NSColor*)backgroundColor;
@end

// Scroll view that contains the main view (where the buttons go).
@interface BookmarkBarFolderWindowScrollView : NSScrollView
@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_WINDOW_H_
