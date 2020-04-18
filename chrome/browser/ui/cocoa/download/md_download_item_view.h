// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_MD_DOWNLOAD_ITEM_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_MD_DOWNLOAD_ITEM_VIEW_H_

#import <AppKit/AppKit.h>

#import "chrome/browser/ui/cocoa/download/download_item_view_protocol.h"

// MDDownloadItemView displays an individual download in the shelf.
@interface MDDownloadItemView : NSView<DownloadItemView>
- (instancetype)init NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithFrame:(NSRect)frameRect NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)coder NS_UNAVAILABLE;
@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_MD_DOWNLOAD_ITEM_VIEW_H_
