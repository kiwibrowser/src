// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_SHEET_INFO_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_SHEET_INFO_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"

@protocol ConstrainedWindowSheet;

// Information about a single sheet managed by
// ConstrainedWindowSheetController. Note, this is a private class not meant for
// public use.
@interface ConstrainedWindowSheetInfo : NSObject {
 @private
  base::scoped_nsprotocol<id<ConstrainedWindowSheet>> sheet_;
  base::scoped_nsobject<NSView> parentView_;
  base::scoped_nsobject<NSWindow> overlayWindow_;
  BOOL sheetDidShow_;
}

@property(nonatomic, readonly) id<ConstrainedWindowSheet> sheet;
@property(nonatomic, readonly) NSView* parentView;
@property(nonatomic, readonly) NSWindow* overlayWindow;
@property(nonatomic, assign) BOOL sheetDidShow;

// Initializes a info object with for the given |sheet| and associated
// |parentView| and |overlayWindow|.
- (id)initWithSheet:(id<ConstrainedWindowSheet>)sheet
         parentView:(NSView*)parentView
      overlayWindow:(NSWindow*)overlayWindow;

// Hides the sheet and the associated overlay window. Hiding is done in such
// a way as to not disturb the window cycle order.
- (void)hideSheet;

// Shows the sheet and the associated overlay window.
- (void)showSheet;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_SHEET_INFO_H_
