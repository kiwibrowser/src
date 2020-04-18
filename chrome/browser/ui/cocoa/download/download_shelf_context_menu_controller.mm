// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/download_shelf_context_menu_controller.h"

#import "chrome/browser/ui/cocoa/download/download_item_controller.h"

@implementation DownloadShelfContextMenuController

- (id)initWithItemController:(DownloadItemController*)itemController
                withDelegate:(id<NSMenuDelegate>)menuDelegate {
  if ((self = [super initWithModel:[itemController contextMenuModel]
                     useWithPopUpButtonCell:NO])) {
    // Retain itemController since the lifetime of the ui::MenuModel is bound to
    // the lifetime of itemController.
    itemController_.reset([itemController retain]);
    menuDelegate_ = menuDelegate;
  }
  return self;
}

- (void)menuDidClose:(NSMenu*)menu {
  [menuDelegate_ menuDidClose:menu];
  [super menuDidClose:menu];
}

@end
