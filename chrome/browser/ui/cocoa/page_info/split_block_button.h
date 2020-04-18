// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PAGE_INFO_SPLIT_BLOCK_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_PAGE_INFO_SPLIT_BLOCK_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_button.h"
#include "ui/base/models/simple_menu_model.h"

@class SplitButtonPopUpCell;
@class SplitButtonTitleCell;

// Block ('deny') button for the permissions bubble.  Subclassed from
// ConstrainedWindowButton, so that it shares styling, but contains two cells
// instead of just one.  The left cell behaves as a normal button, and when
// clicked, calls the button's |action| on its |target|.  The right cell behaves
// as a NSPopUpButtonCell, and implements a single-item menu.
@interface SplitBlockButton : ConstrainedWindowButton {
 @private
  base::scoped_nsobject<SplitButtonTitleCell> leftCell_;
  base::scoped_nsobject<SplitButtonPopUpCell> rightCell_;

  ui::ScopedCrTrackingArea leftTrackingArea_;
  ui::ScopedCrTrackingArea rightTrackingArea_;
}

// Designated initializer.
- (id)initWithMenuDelegate:(ui::SimpleMenuModel::Delegate*)menuDelegate;

@end

#endif  // CHROME_BROWSER_UI_COCOA_PAGE_INFO_SPLIT_BLOCK_BUTTON_H_
