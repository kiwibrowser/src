// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_ITEM_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_ITEM_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/download/download_item_view_protocol.h"
#import "chrome/browser/ui/cocoa/draggable_button.h"
#import "chrome/browser/ui/cocoa/themed_window.h"

@class DownloadItemController;

// A button that is a drag source for a file and that displays a context menu
// instead of firing an action when clicked in a certain area.
@interface DownloadItemButton
    : DraggableButton<NSMenuDelegate, ThemedWindowDrawing, DownloadItemView> {
 @private
  base::FilePath downloadPath_;
  DownloadItemController* controller_;  // weak
  base::scoped_nsobject<NSMenu> contextMenu_;
}

// Shows the DownloadItemButton's context menu.
- (void)showContextMenu;

// Overridden from DraggableButton.
- (void)beginDrag:(NSEvent*)event;

@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_ITEM_BUTTON_H_
