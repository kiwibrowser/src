// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/download/download_shelf_mac.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

// A fake implementation of DownloadShelfController. It implements only the
// methods that DownloadShelfMac call during the tests in this file. We get this
// class into the DownloadShelfMac constructor by some questionable casting --
// Objective C is a dynamic language, so we pretend that's ok.

@interface FakeDownloadShelfController : NSObject {
 @public
  int callCountIsVisible;
  int callCountShow;
  int callCountHide;
  int callCountCloseWithUserAction;
}

- (BOOL)isVisible;
- (void)showDownloadShelf:(BOOL)enable
             isUserAction:(BOOL)isUserAction
                  animate:(BOOL)animate;
@end

@implementation FakeDownloadShelfController

- (BOOL)isVisible {
  ++callCountIsVisible;
  return YES;
}

- (void)showDownloadShelf:(BOOL)enable
             isUserAction:(BOOL)isUserAction
                  animate:(BOOL)animate {
  if (enable)
    ++callCountShow;
  else
    ++callCountHide;
  if (isUserAction && !enable)
    ++callCountCloseWithUserAction;
}

@end


namespace {

class DownloadShelfMacTest : public CocoaProfileTest {
  void SetUp() override {
    CocoaProfileTest::SetUp();
    shelf_controller_.reset([[FakeDownloadShelfController alloc] init]);
  }

 protected:
  base::scoped_nsobject<FakeDownloadShelfController> shelf_controller_;
};

TEST_F(DownloadShelfMacTest, CreationDoesNotCallShow) {
  // Also make sure the DownloadShelfMacTest constructor doesn't crash.
  DownloadShelfMac shelf(browser(),
      (DownloadShelfController*)shelf_controller_.get());
  EXPECT_EQ(0, shelf_controller_.get()->callCountShow);
}

TEST_F(DownloadShelfMacTest, ForwardsShow) {
  DownloadShelfMac shelf(browser(),
      (DownloadShelfController*)shelf_controller_.get());
  EXPECT_EQ(0, shelf_controller_.get()->callCountShow);
  shelf.Open();
  EXPECT_EQ(1, shelf_controller_.get()->callCountShow);
}

TEST_F(DownloadShelfMacTest, ForwardsHide) {
  DownloadShelfMac shelf(browser(),
      (DownloadShelfController*)shelf_controller_.get());
  EXPECT_EQ(0, shelf_controller_.get()->callCountHide);
  shelf.Close(DownloadShelf::AUTOMATIC);
  EXPECT_EQ(1, shelf_controller_.get()->callCountHide);
  EXPECT_EQ(0, shelf_controller_.get()->callCountCloseWithUserAction);
}

TEST_F(DownloadShelfMacTest, ForwardsHideWithUserAction) {
  DownloadShelfMac shelf(browser(),
      (DownloadShelfController*)shelf_controller_.get());
  EXPECT_EQ(0, shelf_controller_.get()->callCountHide);
  shelf.Close(DownloadShelf::USER_ACTION);
  EXPECT_EQ(1, shelf_controller_.get()->callCountHide);
  EXPECT_EQ(1, shelf_controller_.get()->callCountCloseWithUserAction);
}

TEST_F(DownloadShelfMacTest, ForwardsIsShowing) {
  DownloadShelfMac shelf(browser(),
      (DownloadShelfController*)shelf_controller_.get());
  EXPECT_EQ(0, shelf_controller_.get()->callCountIsVisible);
  shelf.IsShowing();
  EXPECT_EQ(1, shelf_controller_.get()->callCountIsVisible);
}

}  // namespace
