// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_ITEM_CELL_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_ITEM_CELL_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/time/time.h"
#import "chrome/browser/ui/cocoa/gradient_button_cell.h"

class DownloadItemModel;
@class IndeterminateProgressTimer;

// A button cell that implements the weird button/popup button hybrid that is
// used by the download items.

// The button represented by this cell consists of a button part on the left
// and a dropdown-menu part on the right. This enum describes which part the
// mouse cursor is over currently.
enum DownloadItemMousePosition {
  kDownloadItemMouseOutside,
  kDownloadItemMouseOverButtonPart,
  kDownloadItemMouseOverDropdownPart
};

@interface DownloadItemCell : GradientButtonCell<NSAnimationDelegate> {
 @private
  // Track which part of the button the mouse is over
  DownloadItemMousePosition mousePosition_;
  int mouseInsideCount_;
  base::scoped_nsobject<NSTrackingArea> trackingAreaButton_;
  base::scoped_nsobject<NSTrackingArea> trackingAreaDropdown_;

  base::FilePath downloadPath_;  // stored unelided
  NSString* secondaryTitle_;
  NSFont* secondaryFont_;
  int percentDone_;
  base::scoped_nsobject<NSAnimation> completionAnimation_;

  base::TimeTicks progressStartTime_;
  base::scoped_nsobject<IndeterminateProgressTimer> indeterminateProgressTimer_;

  BOOL isStatusTextVisible_;
  CGFloat titleY_;
  CGFloat statusAlpha_;
  base::scoped_nsobject<NSAnimation> toggleStatusVisibilityAnimation_;

  std::unique_ptr<ui::ThemeProvider> themeProvider_;
}

@property(nonatomic, copy) NSString* secondaryTitle;
@property(nonatomic, retain) NSFont* secondaryFont;

- (void)setStateFromDownload:(DownloadItemModel*)downloadModel;

// Returns if the mouse is over the button part of the cell.
- (BOOL)isMouseOverButtonPart;

@end

@interface DownloadItemCell(TestingAPI)
- (BOOL)isStatusTextVisible;
- (CGFloat)statusTextAlpha;
- (CGFloat)titleY;
- (void)skipVisibilityAnimation;
- (void)showSecondaryTitle;
- (void)hideSecondaryTitle;
- (IndeterminateProgressTimer*)indeterminateProgressTimer;
@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_ITEM_CELL_H_
