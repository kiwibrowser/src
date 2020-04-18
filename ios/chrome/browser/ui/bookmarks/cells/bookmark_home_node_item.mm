// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/cells/bookmark_home_node_item.h"

#include "base/mac/foundation_util.h"
#include "components/bookmarks/browser/bookmark_node.h"
#import "ios/chrome/browser/ui/bookmarks/cells/bookmark_table_cell.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BookmarkHomeNodeItem
@synthesize bookmarkNode = _bookmarkNode;

- (instancetype)initWithType:(NSInteger)type
                bookmarkNode:(const bookmarks::BookmarkNode*)node {
  if ((self = [super initWithType:type])) {
    self.cellClass = [BookmarkTableCell class];
    _bookmarkNode = node;
  }
  return self;
}

- (void)configureCell:(UITableViewCell*)cell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:cell withStyler:styler];
  BookmarkTableCell* bookmarkCell =
      base::mac::ObjCCastStrict<BookmarkTableCell>(cell);
  [bookmarkCell setNode:self.bookmarkNode];
}

@end
