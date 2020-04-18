// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHOW_ALL_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHOW_ALL_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/themed_window.h"

// The "Show All" button on the download shelf. This is a subclass for custom
// -sizeToFit logic.
@interface DownloadShowAllButton : NSButton<ThemedWindowDrawing>
@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_DOWNLOAD_SHOW_ALL_BUTTON_H_
