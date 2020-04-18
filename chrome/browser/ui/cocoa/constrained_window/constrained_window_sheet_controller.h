// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_SHEET_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_SHEET_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <memory>
#include <vector>

#include "base/mac/scoped_nsobject.h"

namespace web_modal {
class WebContentsModalDialogHost;
}

class WebContentsModalDialogHostCocoa;

@protocol ConstrainedWindowSheet;

// This class manages multiple tab modal sheets for a single parent window. Each
// tab can have a single sheet and only the active tab's sheet will be visible.
// A tab in this case is the |parentView| passed to |-showSheet:forParentView:|.
@interface ConstrainedWindowSheetController : NSObject {
 @private
  base::scoped_nsobject<NSMutableArray> sheets_;
  base::scoped_nsobject<NSWindow> parentWindow_;
  base::scoped_nsobject<NSView> activeView_;

  // Flag to prevent the sheet from updating its position if it's hidden during
  // fullscreen. Otherwise, we will get janky movements during the animation.
  BOOL isSheetHiddenForFullscreen_;

  // Class that bridges the cross-platform web_modal APIs to the Cocoa sheet
  // controller.
  std::unique_ptr<WebContentsModalDialogHostCocoa> dialogHost_;
}

@property(readonly, nonatomic)
    web_modal::WebContentsModalDialogHost* dialogHost;
@property(readonly, nonatomic) NSWindow* parentWindow;

// Returns a sheet controller for |parentWindow|. If a sheet controller does not
// exist yet then one will be created.
+ (ConstrainedWindowSheetController*)
    controllerForParentWindow:(NSWindow*)parentWindow;

// Find a controller that's managing the given sheet. If no such controller
// exists then nil is returned.
+ (ConstrainedWindowSheetController*)
    controllerForSheet:(id<ConstrainedWindowSheet>)sheet;

// Find the sheet attached to the given overlay window.
+ (id<ConstrainedWindowSheet>)sheetForOverlayWindow:(NSWindow*)overlayWindow;

// Shows the given sheet over |parentView|.
- (void)showSheet:(id<ConstrainedWindowSheet>)sheet
    forParentView:(NSView*)parentView;

// Hides |sheet| over the active view.
- (void)hideSheet:(id<ConstrainedWindowSheet>)sheet;

// Hides and unhides the sheet at the beginning and end of fullscreen
// transition. |hideSheetForFullscreenTransition| gets called at the beginning
// of the transition and |unhideSheetForFullscreenTransition| gets called at
// the end.
- (void)hideSheetForFullscreenTransition;
- (void)unhideSheetForFullscreenTransition;

// Calculates the position of the sheet for the given window size.
- (NSPoint)originForSheet:(id<ConstrainedWindowSheet>)sheet
           withWindowSize:(NSSize)size;

// Closes the given sheet.
- (void)closeSheet:(id<ConstrainedWindowSheet>)sheet;

// Run a pulse animation for the given sheet. This does nothing if the sheet
// is not visible.
- (void)pulseSheet:(id<ConstrainedWindowSheet>)sheet;

// Gets the number of sheets attached to the controller's window.
- (int)sheetCount;

// The size of the overlay window, which can be used to determine a preferred
// maximum size for a dialog that should be contained within |parentView|.
- (NSSize)overlayWindowSizeForParentView:(NSView*)parentView;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_SHEET_CONTROLLER_H_
