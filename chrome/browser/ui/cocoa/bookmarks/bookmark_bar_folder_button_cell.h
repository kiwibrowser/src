// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_BUTTON_CELL_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_BUTTON_CELL_H_

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button_cell.h"

// A button cell that handles drawing/highlighting of buttons in the
// bookmark bar.  This cell forwards mouseEntered/mouseExited events
// to its control view so that pseudo-menu operations
// (e.g. hover-over to open) can be implemented.
@interface BookmarkBarFolderButtonCell : BookmarkButtonCell

// Create a button cell which draws without a theme and with a frame
// color provided by the ThemeService defaults.
+ (id)buttonCellForNode:(const bookmarks::BookmarkNode*)node
                   text:(NSString*)text
                  image:(NSImage*)image
         menuController:(BookmarkContextMenuCocoaController*)menuController;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_BUTTON_CELL_H_
