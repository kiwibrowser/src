// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_TREE_BROWSER_CELL_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_TREE_BROWSER_CELL_H_

#import <Cocoa/Cocoa.h>

namespace bookmarks {
class BookmarkNode;
}

// Provides a custom cell as used in the BookmarkEditor.xib's folder tree
// browser view.  This cell customization adds target and action support
// not provided by the NSBrowserCell as well as contextual information
// identifying the bookmark node being edited and the column matrix
// control in which is contained the cell.
@interface BookmarkTreeBrowserCell : NSBrowserCell {
 @private
  const bookmarks::BookmarkNode* bookmarkNode_;  // weak
  NSMatrix* matrix_;  // weak

  // NSCell does not implement the |target| or |action| properties. Subclasses
  // that need this functionality are expected to implement this functionality.
  id target_;  // weak
  SEL action_;
}

@property(nonatomic, assign) NSMatrix* matrix;

#if !defined(MAC_OS_X_VERSION_10_10) || \
    MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_10
// In OSX SDK <= 10.9, there are setters and getters for target and action in
// NSCell, but no properties. In the OSX 10.10 SDK, the properties are defined
// as atomic. There is no point in redeclaring the properties if they already
// exist.
@property(nonatomic, assign) id target;
@property(nonatomic, assign) SEL action;
#endif  // MAC_OS_X_VERSION_10_10

- (const bookmarks::BookmarkNode*)bookmarkNode;
- (void)setBookmarkNode:(const bookmarks::BookmarkNode*)bookmarkNode;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_TREE_BROWSER_CELL_H_
