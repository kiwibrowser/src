// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHELF_VIEW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHELF_VIEW_COCOA_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/animatable_view.h"

@class HoverCloseButton;

// A view that handles any special rendering for the download shelf, painting
// a gradient and managing a set of DownloadItemViews.

@interface DownloadShelfView : AnimatableView {
 @private
  IBOutlet HoverCloseButton* closeButton_;
}
+ (CGFloat)shelfHeight;
@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHELF_VIEW_COCOA_H_
