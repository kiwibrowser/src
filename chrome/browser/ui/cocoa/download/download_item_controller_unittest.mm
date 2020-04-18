// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/download_item_controller.h"

#import <Cocoa/Cocoa.h>

#include <memory>

#import "base/mac/scoped_nsobject.h"
#include "base/run_loop.h"
#import "chrome/browser/ui/cocoa/download/download_shelf_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/common/chrome_features.h"
#include "components/download/public/common/mock_download_item.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"
#include "ui/events/test/cocoa_test_event_utils.h"

using ::testing::Return;
using ::testing::ReturnRefOfCopy;

@interface DownloadItemController (DownloadItemControllerTest)
- (void)verifyProgressViewIsVisible:(bool)visible;
- (void)verifyDangerousDownloadPromptIsVisible:(bool)visible;
@end

@implementation DownloadItemController (DownloadItemControllerTest)
- (void)verifyProgressViewIsVisible:(bool)visible {
  EXPECT_EQ(visible, ![progressView_ isHidden]);
}

- (void)verifyDangerousDownloadPromptIsVisible:(bool)visible {
  EXPECT_EQ(visible, ![dangerousDownloadView_ isHidden]);
}
@end

namespace {

class DownloadItemControllerTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    download_item_.reset(new ::testing::NiceMock<download::MockDownloadItem>);
    ON_CALL(*download_item_, GetState())
        .WillByDefault(Return(download::DownloadItem::IN_PROGRESS));
    ON_CALL(*download_item_, GetFullPath())
        .WillByDefault(ReturnRefOfCopy(base::FilePath()));
    ON_CALL(*download_item_, GetFileNameToReportUser())
        .WillByDefault(Return(base::FilePath()));
    ON_CALL(*download_item_, GetDangerType())
        .WillByDefault(Return(download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS));
    ON_CALL(*download_item_, GetTargetFilePath())
        .WillByDefault(ReturnRefOfCopy(base::FilePath()));
    ON_CALL(*download_item_, GetLastReason())
        .WillByDefault(Return(download::DOWNLOAD_INTERRUPT_REASON_NONE));
    ON_CALL(*download_item_, GetURL()).WillByDefault(ReturnRefOfCopy(GURL()));
    ON_CALL(*download_item_, GetReferrerUrl())
        .WillByDefault(ReturnRefOfCopy(GURL()));
    ON_CALL(*download_item_, GetTargetDisposition())
        .WillByDefault(
            Return(download::DownloadItem::TARGET_DISPOSITION_OVERWRITE));

    id shelf_controller =
        [OCMockObject mockForClass:[DownloadShelfController class]];
    shelf_controller_.reset([shelf_controller retain]);

    item_controller_.reset([[DownloadItemController alloc]
        initWithDownload:download_item_.get()
               navigator:NULL]);
    [item_controller_ setShelf:shelf_controller_];
    [[test_window() contentView] addSubview:[item_controller_ view]];
  }

  void TearDown() override {
    [(id)shelf_controller_ verify];
    [item_controller_ setShelf:nil];
    CocoaProfileTest::TearDown();
  }

 protected:
  std::unique_ptr<download::MockDownloadItem> download_item_;
  base::scoped_nsobject<DownloadShelfController> shelf_controller_;
  base::scoped_nsobject<DownloadItemController> item_controller_;
};

TEST_F(DownloadItemControllerTest, ShelfNotifiedOfOpenedDownload) {
  [[(id)shelf_controller_ expect] downloadWasOpened:[OCMArg any]];
  download_item_->NotifyObserversDownloadOpened();
}

TEST_F(DownloadItemControllerTest, RemovesSelfWhenDownloadIsDestroyed) {
  [[(id)shelf_controller_ expect] remove:[OCMArg any]];
  download_item_.reset();
}

TEST_F(DownloadItemControllerTest, NormalDownload) {
  // In MD, this is handled by the MDDownloadItemView.
  if (base::FeatureList::IsEnabled(features::kMacMaterialDesignDownloadShelf))
    return;

  [item_controller_ verifyProgressViewIsVisible:true];
  [item_controller_ verifyDangerousDownloadPromptIsVisible:false];
}

TEST_F(DownloadItemControllerTest, DangerousDownload) {
  // In MD, this is handled by the MDDownloadItemView.
  if (base::FeatureList::IsEnabled(features::kMacMaterialDesignDownloadShelf))
    return;

  ON_CALL(*download_item_, GetDangerType())
      .WillByDefault(Return(download::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE));
  ON_CALL(*download_item_, IsDangerous()).WillByDefault(Return(true));
  [[(id)shelf_controller_ expect] layoutItems];
  download_item_->NotifyObserversDownloadUpdated();

  [item_controller_ verifyProgressViewIsVisible:false];
  [item_controller_ verifyDangerousDownloadPromptIsVisible:true];
}

TEST_F(DownloadItemControllerTest, NormalDownloadBecomesDangerous) {
  // In MD, this is handled by the MDDownloadItemView.
  if (base::FeatureList::IsEnabled(features::kMacMaterialDesignDownloadShelf))
    return;

  [item_controller_ verifyProgressViewIsVisible:true];
  [item_controller_ verifyDangerousDownloadPromptIsVisible:false];

  // The download is now marked as dangerous.
  [[(id)shelf_controller_ expect] layoutItems];
  ON_CALL(*download_item_, GetDangerType())
      .WillByDefault(Return(download::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE));
  ON_CALL(*download_item_, IsDangerous()).WillByDefault(Return(true));
  download_item_->NotifyObserversDownloadUpdated();

  [item_controller_ verifyProgressViewIsVisible:false];
  [item_controller_ verifyDangerousDownloadPromptIsVisible:true];
  [(id)shelf_controller_ verify];

  // And then marked as safe again.
  [[(id)shelf_controller_ expect] layoutItems];
  ON_CALL(*download_item_, GetDangerType())
      .WillByDefault(Return(download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS));
  ON_CALL(*download_item_, IsDangerous()).WillByDefault(Return(false));
  download_item_->NotifyObserversDownloadUpdated();

  [item_controller_ verifyProgressViewIsVisible:true];
  [item_controller_ verifyDangerousDownloadPromptIsVisible:false];
  [(id)shelf_controller_ verify];
}

TEST_F(DownloadItemControllerTest, DismissesContextMenuWhenRemovedFromWindow) {
  // In MD, this is handled by the MDDownloadItemView.
  if (base::FeatureList::IsEnabled(features::kMacMaterialDesignDownloadShelf))
    return;

  // showContextMenu: calls [NSMenu popUpContextMenu:...], which blocks until
  // the menu is dismissed. Use a block to cancel the menu while waiting for
  // [NSMenu popUpContextMenu:...] to return (this block will execute on the
  // main thread, on the next pass through the run loop).
  __block bool did_block = false;
  [[NSOperationQueue mainQueue] addOperationWithBlock:^{
    // Simulate the item's removal from the shelf. Ideally we would call an
    // actual shelf removal method like [item_controller_ remove] but the shelf
    // and download item are mock objects.
    [[item_controller_ view] removeFromSuperview];
    did_block = true;
  }];
  // The unit test will stop here until the block causes the DownloadItemButton
  // to dismiss the menu.
  [item_controller_ showContextMenu:nil];
  // Otherwise, the menu was probably never shown.
  EXPECT_TRUE(did_block);
}

// https://crbug.com/815161: Deleting the controller while the mouse button is
// down should not crash. This crash only affects macOS 10.12 and older.
TEST_F(DownloadItemControllerTest, DeleteWithMouseDownDoesNotCrash) {
  NSView* item_view = [item_controller_ view];
  NSRect item_frame = [item_view convertRect:item_view.bounds toView:nil];
  NSPoint item_center = NSMakePoint(NSMidX(item_frame), NSMidY(item_frame));
  NSEvent* mouse_down_event =
      cocoa_test_event_utils::LeftMouseDownAtPointInWindow(item_center,
                                                           test_window());
  NSEvent* mouse_up_event =
      cocoa_test_event_utils::MouseEventWithType(NSLeftMouseUp, 0);

  // Verify that the pair of events activates the button (else this isn't
  // testing anything).
  EXPECT_CALL(*download_item_, OpenDownload());
  [NSApp postEvent:mouse_up_event atStart:NO];
  [NSApp sendEvent:mouse_down_event];
  testing::Mock::VerifyAndClearExpectations(download_item_.get());

  [[NSOperationQueue mainQueue] addOperationWithBlock:^{
    [[item_controller_ view] removeFromSuperview];
    item_controller_.reset();
    [NSApp postEvent:mouse_up_event atStart:NO];
  }];
  [NSApp sendEvent:mouse_down_event];

  // Verify that the block ran. At this point, the test would have crashed.
  EXPECT_EQ(nullptr, item_controller_);
}

}  // namespace
