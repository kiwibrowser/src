// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DOWNLOAD_MD_DOWNLOAD_ITEM_VIEW_TESTING_H_
#define CHROME_BROWSER_UI_COCOA_DOWNLOAD_MD_DOWNLOAD_ITEM_VIEW_TESTING_H_

#import "chrome/browser/ui/cocoa/download/md_download_item_view.h"

@interface MDDownloadItemView (Testing)
@property(readonly) NSButton* primaryButton;
@property(readonly) NSButton* menuButton;
@property(readonly) NSView* dangerView;
@end

#endif  // CHROME_BROWSER_UI_COCOA_DOWNLOAD_MD_DOWNLOAD_ITEM_VIEW_TESTING_H_
