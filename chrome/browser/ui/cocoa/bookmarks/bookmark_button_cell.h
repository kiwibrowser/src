// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BUTTON_CELL_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BUTTON_CELL_H_

#import "chrome/browser/ui/cocoa/gradient_button_cell.h"

@class BookmarkContextMenuCocoaController;

namespace bookmarks {
class BookmarkNode;
}

// A button cell that handles drawing/highlighting of buttons in the
// bookmark bar.  This cell forwards mouseEntered/mouseExited events
// to its control view so that pseudo-menu operations
// (e.g. hover-over to open) can be implemented.
@interface BookmarkButtonCell : GradientButtonCell<NSMenuDelegate> {
 @private
  // Controller for showing the context menu. Weak, owned by
  // BookmarkBarController.
  BookmarkContextMenuCocoaController* menuController_;

  BOOL empty_;  // is this an "empty" button placeholder button cell?

  // Starting index of bookmarkFolder children that we care to use.
  int startingChildIndex_;

  // Should we draw the folder arrow as needed?  Not used for the bar
  // itself but used on the folder windows.
  BOOL drawFolderArrow_;

  // Arrow for folders
  base::scoped_nsobject<NSImage> arrowImage_;

  // Text color for title.
  base::scoped_nsobject<NSColor> textColor_;
}

@property(nonatomic, readwrite, assign)
    const bookmarks::BookmarkNode* bookmarkNode;
@property(nonatomic, readwrite, assign) int startingChildIndex;
@property(nonatomic, readwrite, assign) BOOL drawFolderArrow;

// Returns the width of a cell for the given node and image for
// display on the bookmark bar (meaning no arrow for folder
// nodes). Used for laying out bookmark bar without creating
// live cells.
+ (CGFloat)cellWidthForNode:(const bookmarks::BookmarkNode*)node
                      image:(NSImage*)image;

// Create a button cell which draws with a theme.
+ (id)buttonCellForNode:(const bookmarks::BookmarkNode*)node
                   text:(NSString*)text
                  image:(NSImage*)image
         menuController:(BookmarkContextMenuCocoaController*)menuController;

// Create a button cell not attached to any node which draws with a theme.
+ (id)buttonCellWithText:(NSString*)text
                   image:(NSImage*)image
          menuController:(BookmarkContextMenuCocoaController*)menuController;

// Create an |OffTheSideButtonCell| (aka the overflow chevron.)
+ (id)offTheSideButtonCell;

// Initialize a button cell which draws with a theme.
// Designated initializer.
- (id)initForNode:(const bookmarks::BookmarkNode*)node
              text:(NSString*)text
             image:(NSImage*)image
    menuController:(BookmarkContextMenuCocoaController*)menuController;

// Initialize a button cell not attached to any node which draws with a theme.
- (id)initWithText:(NSString*)text
             image:(NSImage*)image
    menuController:(BookmarkContextMenuCocoaController*)menuController;

// A button cell is considered empty if it is expected to be attached to a node
// and this node is NULL. If the button was created with
// buttonCellForContextMenu then no node is expected and empty is always NO.
- (BOOL)empty;
- (void)setEmpty:(BOOL)empty;

// |-setBookmarkCellText:image:| is used to set the text and image of
// a BookmarkButtonCell, and align the image to the left (NSImageLeft)
// if there is text in the title, and centered (NSImageCenter) if
// there is not.  If |title| is nil, do not reset the title.
- (void)setBookmarkCellText:(NSString*)title
                      image:(NSImage*)image;

// Set the color of text in this cell.
- (void)setTextColor:(NSColor*)color;

- (BOOL)isFolderButtonCell;
- (void)setBookmarkNode:(const bookmarks::BookmarkNode*)node
                  image:(NSImage*)image;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BUTTON_CELL_H_
