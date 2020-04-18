// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_button_cell.h"

#include "ui/base/material_design/material_design_controller.h"

using bookmarks::BookmarkNode;

@implementation BookmarkBarFolderButtonCell

+ (id)buttonCellForNode:(const BookmarkNode*)node
                   text:(NSString*)text
                  image:(NSImage*)image
         menuController:(BookmarkContextMenuCocoaController*)menuController {
  id buttonCell =
      [[[BookmarkBarFolderButtonCell alloc] initForNode:node
                                                   text:text
                                                  image:image
                                         menuController:menuController]
       autorelease];
  return buttonCell;
}

- (BOOL)isFolderButtonCell {
  return YES;
}

- (void)setMouseInside:(BOOL)flag animate:(BOOL)animated {
}

- (int)verticalTextOffset {
  return -3;
}

@end
