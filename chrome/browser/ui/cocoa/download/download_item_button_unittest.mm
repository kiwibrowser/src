// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/download_item_button.h"
#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/download/download_item_model.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "components/download/public/common/mock_download_item.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

// Make sure nothing leaks.
TEST(DownloadItemButtonTest, Create) {
  base::scoped_nsobject<DownloadItemButton> button;
  button.reset([[DownloadItemButton alloc]
      initWithFrame:NSMakeRect(0,0,500,500)]);

  // Test setter
  testing::NiceMock<download::MockDownloadItem> download;
  DownloadItemModel download_model(&download);
  [button.get() setStateFromDownload:&download_model];
}
