// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHELF_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHELF_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/has_weak_browser_pointer.h"
#import "chrome/browser/ui/cocoa/view_resizer.h"
#include "ui/base/cocoa/tracking_area.h"

@class AnimatableView;
class Browser;
@class BrowserWindowController;
@class DownloadItemController;
class DownloadShelf;
@class DownloadShelfView;
@class HyperlinkButtonCell;
@class HoverButton;

namespace content {
class PageNavigator;
}

namespace download {
class DownloadItem;
}

// A controller class that manages the download shelf for one window. It is
// responsible for the behavior of the shelf itself (showing/hiding, handling
// the link, layout) as well as for managing the download items it contains.
//
// All the files in cocoa/downloads_* are related as follows:
//
// download_shelf_mac bridges calls from chromium's c++ world to the objc
// download_shelf_controller for the shelf (this file). The shelf's background
// is drawn by download_shelf_view. Every item in a shelf is controlled by a
// download_item_controller.
//
// download_item_mac bridges calls from chromium's c++ world to the objc
// download_item_controller, which is responsible for managing a single item
// on the shelf. The item controller loads its UI from a xib file, where the
// UI of an item itself is represented by a button that is drawn by
// download_item_cell.

@interface DownloadShelfController
    : NSViewController<NSTextViewDelegate, HasWeakBrowserPointer> {
 @private
  IBOutlet HoverButton* hoverCloseButton_;

  // YES if the download shelf is intended to be displayed. The shelf animates
  // out when it is closing. During this time, barIsVisible_ is NO although the
  // shelf is still visible on screen.
  BOOL barIsVisible_;

  // YES if the containing browser window is fullscreen.
  BOOL isFullscreen_;

  // YES if the shelf should be closed when the mouse leaves the shelf.
  BOOL shouldCloseOnMouseExit_;

  // YES if the mouse is currently over the download shelf.
  BOOL isMouseInsideView_;

  std::unique_ptr<DownloadShelf> bridge_;

  // Height of the shelf when it's fully visible.
  CGFloat maxShelfHeight_;

  // Current height of the shelf. Changes while the shelf is animating in or
  // out.
  CGFloat currentShelfHeight_;

  // Used to autoclose the shelf when the mouse is moved off it.
  ui::ScopedCrTrackingArea trackingArea_;

  // The download items we have added to our shelf.
  base::scoped_nsobject<NSMutableArray> downloadItemControllers_;

  // The container that contains (and clamps) all the download items.
  IBOutlet NSView* itemContainerView_;

  // Delegate that handles resizing our view.
  id<ViewResizer> resizeDelegate_;

  // Used for loading pages.
  content::PageNavigator* navigator_;
};

- (id)initWithBrowser:(Browser*)browser
       resizeDelegate:(id<ViewResizer>)resizeDelegate;

// Run when the user clicks the 'Show All' button.
- (IBAction)showDownloadsTab:(id)sender;

// Run when the user clicks the close button on the right side of the shelf.
- (IBAction)handleClose:(id)sender;

// Shows or hides the download shelf based on the value of |show|.
// |isUserAction| should be YES if the operation is being triggered based on a
// user action (currently only relevant when hiding the shelf).
// Note: This is intended to be invoked from DownloadShelfMac. If invoked
// directly, the shelf visibility state maintained by DownloadShelf and the
// owning Browser will not be updated.
- (void)showDownloadShelf:(BOOL)show
             isUserAction:(BOOL)isUserAction
                  animate:(BOOL)animate;

// Returns our view cast as an AnimatableView.
- (AnimatableView*)animatableView;

- (DownloadShelf*)bridge;
- (BOOL)isVisible;

// Add a new download item to the leftmost position of the download shelf. The
// item should not have been already added to this shelf.
- (void)addDownloadItem:(download::DownloadItem*)downloadItem;

// Similar to addDownloadItem above, but adds a DownloadItemController.
- (void)add:(DownloadItemController*)download;

// Remove a download, possibly via clearing browser data.
- (void)remove:(DownloadItemController*)download;

// Called by individual item controllers when their downloads are opened.
- (void)downloadWasOpened:(DownloadItemController*)download;

// Return the height of the download shelf.
- (float)height;

// Re-layouts all download items based on their current state.
- (void)layoutItems;

@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHELF_CONTROLLER_H_
