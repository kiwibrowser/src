// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHELF_CONTEXT_MENU_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHELF_CONTEXT_MENU_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/menu_controller.h"
#include "base/mac/scoped_nsobject.h"

@class DownloadItemController;

// MenuControllerCocoa that retains a DownloadItemController and instantiates a
// menu based on the contextMenuModel from the DownloadItemController.
@interface DownloadShelfContextMenuController : MenuControllerCocoa {
 @private
  base::scoped_nsobject<DownloadItemController> itemController_;
  id<NSMenuDelegate> menuDelegate_;
}

// Initializes the MenuControllerCocoa using [itemController contextMenuModel].
// menuDelegate will be sent a menuDidClose message when the menu closes.
- (id)initWithItemController:(DownloadItemController*)itemController
                withDelegate:(id<NSMenuDelegate>)menuDelegate;

- (void)menuDidClose:(NSMenu*)menu;
@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHELF_CONTEXT_MENU_CONTROLLER_H_
