// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/download_shelf_controller.h"

#import <Cocoa/Cocoa.h>

#include <memory>
#include <utility>

#import "base/mac/scoped_block.h"
#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/download/download_shelf.h"
#import "chrome/browser/ui/cocoa/download/download_item_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#import "chrome/browser/ui/cocoa/view_resizer_pong.h"
#include "components/download/public/common/mock_download_item.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"
#import "ui/events/test/cocoa_test_event_utils.h"

using ::testing::Return;
using ::testing::AnyNumber;

// Wraps a download::MockDownloadItem so it can be retained by the mock
// DownloadItemController.
@interface WrappedMockDownloadItem : NSObject {
 @private
  std::unique_ptr<download::MockDownloadItem> download_;
}
- (id)initWithMockDownload:
    (std::unique_ptr<download::MockDownloadItem>)download;
- (download::DownloadItem*)download;
- (download::MockDownloadItem*)mockDownload;
@end

@implementation WrappedMockDownloadItem
- (id)initWithMockDownload:
    (std::unique_ptr<download::MockDownloadItem>)download {
  if ((self = [super init])) {
    download_ = std::move(download);
  }
  return self;
}

- (download::DownloadItem*)download {
  return download_.get();
}

- (download::MockDownloadItem*)mockDownload {
  return download_.get();
}
@end

// Test method for accessing the wrapped MockDownloadItem.
@interface DownloadItemController (DownloadShelfControllerTest) {
}
- (WrappedMockDownloadItem*)wrappedMockDownload;
@end

@implementation DownloadItemController (DownloadShelfControllerTest)
- (WrappedMockDownloadItem*)wrappedMockDownload {
  return nil;
}
@end

@interface DownloadShelfController (Testing)
- (void)animationDidEnd:(NSAnimation*)animation;
@end

// Subclass of the DownloadShelfController to override scheduleAutoClose and
// cancelAutoClose. During regular operation, a scheduled autoClose waits for 5
// seconds before closing the shelf (unless it is cancelled during this
// time). For testing purposes, we count the number of invocations of
// {schedule,cancel}AutoClose instead of actually scheduling and cancelling.
@interface CountingDownloadShelfController : DownloadShelfController {
 @public
  int scheduleAutoCloseCount_;
  int cancelAutoCloseCount_;
  base::mac::ScopedBlock<dispatch_block_t> closeAnimationHandler_;
}

// Handler will be called at the end of a close animation.
- (void)setCloseAnimationHandler:(dispatch_block_t)handler;
@end

@implementation CountingDownloadShelfController

-(void)scheduleAutoClose {
  ++scheduleAutoCloseCount_;
}

-(void)cancelAutoClose {
  ++cancelAutoCloseCount_;
}

- (void)setCloseAnimationHandler:(dispatch_block_t)handler {
  closeAnimationHandler_.reset(handler);
}

- (void)animationDidEnd:(NSAnimation*)animation {
  [super animationDidEnd:animation];
  if (closeAnimationHandler_)
    closeAnimationHandler_.get()();
}

@end

namespace {

class DownloadShelfControllerTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    resize_delegate_.reset([[ViewResizerPong alloc] init]);
    shelf_.reset([[CountingDownloadShelfController alloc]
                   initWithBrowser:browser()
                    resizeDelegate:resize_delegate_.get()]);
    EXPECT_TRUE([shelf_ view]);
    [[test_window() contentView] addSubview:[shelf_ view]];
  }

  void TearDown() override {
    if (shelf_.get()) {
      shelf_.reset();
    }
    CocoaProfileTest::TearDown();
  }

 protected:
  id CreateItemController();

  base::scoped_nsobject<CountingDownloadShelfController> shelf_;
  base::scoped_nsobject<ViewResizerPong> resize_delegate_;
};

id DownloadShelfControllerTest::CreateItemController() {
  std::unique_ptr<download::MockDownloadItem> download(
      new ::testing::NiceMock<download::MockDownloadItem>);
  ON_CALL(*download.get(), GetOpened())
      .WillByDefault(Return(false));
  ON_CALL(*download.get(), GetState())
      .WillByDefault(Return(download::DownloadItem::IN_PROGRESS));

  base::scoped_nsobject<WrappedMockDownloadItem> wrappedMockDownload(
      [[WrappedMockDownloadItem alloc]
          initWithMockDownload:std::move(download)]);

  id item_controller =
      [OCMockObject mockForClass:[DownloadItemController class]];
  base::scoped_nsobject<NSView> view([[NSView alloc] initWithFrame:NSZeroRect]);
  [[[item_controller stub] andCall:@selector(download)
                          onObject:wrappedMockDownload.get()] download];
  [[item_controller stub] updateVisibility:[OCMArg any]];
  [[[item_controller stub]
     andReturnValue:[NSValue valueWithSize:NSMakeSize(10,10)]] preferredSize];
  [[[item_controller stub] andReturn:view.get()] view];
  [[[item_controller stub]
     andReturn:wrappedMockDownload.get()] wrappedMockDownload];
  [[item_controller stub] setShelf:[OCMArg any]];
  return [item_controller retain];
}

TEST_VIEW(DownloadShelfControllerTest, [shelf_ view]);

// Removing the last download from the shelf should cause it to close
// immediately.
TEST_F(DownloadShelfControllerTest, AddAndRemoveDownload) {
  base::scoped_nsobject<DownloadItemController> item(CreateItemController());
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:YES];
  EXPECT_TRUE([shelf_ isVisible]);
  EXPECT_TRUE([shelf_ bridge]->IsShowing());
  [shelf_ add:item];
  [shelf_ remove:item];
  EXPECT_FALSE([shelf_ isVisible]);
  EXPECT_FALSE([shelf_ bridge]->IsShowing());
  // The shelf should be closed without scheduling an autoClose.
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);
}

// Test that the shelf doesn't close automatically after a removal if there are
// active download items still on the shelf.
// Disabled due to flakiness. https://crbug.com/832389
#define MAYBE_AddAndRemoveWithActiveItem DISABLED_AddAndRemoveWithActiveItem
TEST_F(DownloadShelfControllerTest, MAYBE_AddAndRemoveWithActiveItem) {
  base::scoped_nsobject<DownloadItemController> item1(CreateItemController());
  base::scoped_nsobject<DownloadItemController> item2(CreateItemController());
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:YES];
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ add:item1.get()];
  [shelf_ add:item2.get()];
  [shelf_ remove:item1.get()];
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ remove:item2.get()];
  EXPECT_FALSE([shelf_ isVisible]);
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);
}

// DownloadShelf::Unhide() should cause the shelf to be displayed if there are
// active downloads on it.
TEST_F(DownloadShelfControllerTest, DISABLED_HideAndUnhide) {
  base::scoped_nsobject<DownloadItemController> item(CreateItemController());
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:YES];
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ add:item.get()];
  [shelf_ bridge]->Hide();
  EXPECT_FALSE([shelf_ isVisible]);
  [shelf_ bridge]->Unhide();
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ remove:item.get()];
  EXPECT_FALSE([shelf_ isVisible]);
}

// DownloadShelf::Unhide() shouldn't cause the shelf to be displayed if all
// active downloads are removed from the shelf while the shelf was hidden.
TEST_F(DownloadShelfControllerTest, HideAutocloseUnhide) {
  base::scoped_nsobject<DownloadItemController> item(CreateItemController());
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:YES];
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ add:item.get()];
  [shelf_ bridge]->Hide();
  EXPECT_FALSE([shelf_ isVisible]);
  [shelf_ remove:item.get()];
  EXPECT_FALSE([shelf_ isVisible]);
  [shelf_ bridge]->Unhide();
  EXPECT_FALSE([shelf_ isVisible]);
}

// Test of autoclosing behavior after opening a download item. The mouse is on
// the download shelf at the time the autoclose is scheduled.
TEST_F(DownloadShelfControllerTest, AutoCloseAfterOpenWithMouseInShelf) {
  base::scoped_nsobject<DownloadItemController> item(CreateItemController());
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:YES];
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ add:item.get()];
  // Expect 2 cancelAutoClose calls: From the showDownloadShelf: call and the
  // add: call.
  EXPECT_EQ(2, shelf_.get()->cancelAutoCloseCount_);
  shelf_.get()->cancelAutoCloseCount_ = 0;

  // The mouse enters the shelf.
  [shelf_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);

  // The download opens.
  EXPECT_CALL(*[[item wrappedMockDownload] mockDownload], GetOpened())
      .WillRepeatedly(Return(true));
  [shelf_ downloadWasOpened:item.get()];

  // The shelf should now be waiting for the mouse to exit.
  EXPECT_TRUE([shelf_ isVisible]);
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(0, shelf_.get()->cancelAutoCloseCount_);

  // The mouse exits the shelf. autoClose should be scheduled now.
  [shelf_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_EQ(1, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(0, shelf_.get()->cancelAutoCloseCount_);

  // The mouse enters the shelf again. The autoClose should be cancelled.
  [shelf_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_EQ(1, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(1, shelf_.get()->cancelAutoCloseCount_);
}

// Test of autoclosing behavior after opening a download item.
TEST_F(DownloadShelfControllerTest, AutoCloseAfterOpenWithMouseOffShelf) {
  base::scoped_nsobject<DownloadItemController> item(CreateItemController());
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:YES];
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ add:item.get()];

  // The download is opened.
  EXPECT_CALL(*[[item wrappedMockDownload] mockDownload], GetOpened())
      .WillRepeatedly(Return(true));
  [shelf_ downloadWasOpened:item.get()];

  // The shelf should be closed immediately since the mouse is not over the
  // shelf.
  EXPECT_FALSE([shelf_ isVisible]);
}

// Test that if the shelf is closed while an autoClose is pending, the pending
// autoClose is cancelled.
TEST_F(DownloadShelfControllerTest, CloseWithPendingAutoClose) {
  base::scoped_nsobject<DownloadItemController> item(CreateItemController());
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:YES];
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ add:item.get()];
  // Expect 2 cancelAutoClose calls: From the showDownloadShelf: call and the
  // add: call.
  EXPECT_EQ(2, shelf_.get()->cancelAutoCloseCount_);
  shelf_.get()->cancelAutoCloseCount_ = 0;

  // The mouse enters the shelf.
  [shelf_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);

  // The download opens.
  EXPECT_CALL(*[[item wrappedMockDownload] mockDownload], GetOpened())
      .WillRepeatedly(Return(true));
  [shelf_ downloadWasOpened:item.get()];

  // The shelf should now be waiting for the mouse to exit. autoClose should not
  // be scheduled yet.
  EXPECT_TRUE([shelf_ isVisible]);
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(0, shelf_.get()->cancelAutoCloseCount_);

  // The mouse exits the shelf. autoClose should be scheduled now.
  [shelf_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_EQ(1, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(0, shelf_.get()->cancelAutoCloseCount_);

  // Remove the download item. This should cause the download shelf to be hidden
  // immediately. The pending autoClose should be cancelled.
  [shelf_ remove:item];
  EXPECT_EQ(1, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(1, shelf_.get()->cancelAutoCloseCount_);
  EXPECT_FALSE([shelf_ isVisible]);
}

// That that the shelf cancels a pending autoClose if a new download item is
// added to it.
TEST_F(DownloadShelfControllerTest, AddItemWithPendingAutoClose) {
  base::scoped_nsobject<DownloadItemController> item(CreateItemController());
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:YES];
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ add:item.get()];
  // Expect 2 cancelAutoClose calls: From the showDownloadShelf: call and the
  // add: call.
  EXPECT_EQ(2, shelf_.get()->cancelAutoCloseCount_);
  shelf_.get()->cancelAutoCloseCount_ = 0;

  // The mouse enters the shelf.
  [shelf_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);

  // The download opens.
  EXPECT_CALL(*[[item wrappedMockDownload] mockDownload], GetOpened())
      .WillRepeatedly(Return(true));
  [shelf_ downloadWasOpened:item.get()];

  // The shelf should now be waiting for the mouse to exit. autoClose should not
  // be scheduled yet.
  EXPECT_TRUE([shelf_ isVisible]);
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(0, shelf_.get()->cancelAutoCloseCount_);

  // The mouse exits the shelf. autoClose should be scheduled now.
  [shelf_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_EQ(1, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(0, shelf_.get()->cancelAutoCloseCount_);

  // Add a new download item. The pending autoClose should be cancelled.
  base::scoped_nsobject<DownloadItemController> item2(CreateItemController());
  [shelf_ add:item.get()];
  EXPECT_EQ(1, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(1, shelf_.get()->cancelAutoCloseCount_);
  EXPECT_TRUE([shelf_ isVisible]);
}

// Test that pending autoClose calls are cancelled when exiting.
TEST_F(DownloadShelfControllerTest, CancelAutoCloseOnExit) {
  base::scoped_nsobject<DownloadItemController> item(CreateItemController());
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:YES];
  EXPECT_TRUE([shelf_ isVisible]);
  [shelf_ add:item.get()];
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(2, shelf_.get()->cancelAutoCloseCount_);

  [shelf_ browserWillBeDestroyed];
  EXPECT_EQ(0, shelf_.get()->scheduleAutoCloseCount_);
  EXPECT_EQ(3, shelf_.get()->cancelAutoCloseCount_);
  shelf_.reset();
}

// The view should not be hidden when the shelf is open.
// The view should be hidden when the shelf is closed.
TEST_F(DownloadShelfControllerTest, ViewVisibility) {
  [shelf_ showDownloadShelf:YES isUserAction:NO animate:NO];
  EXPECT_FALSE([[shelf_ view] isHidden]);

  [shelf_ showDownloadShelf:NO isUserAction:NO animate:NO];
  EXPECT_TRUE([[shelf_ view] isHidden]);

  [shelf_ showDownloadShelf:YES isUserAction:NO animate:NO];
  EXPECT_FALSE([[shelf_ view] isHidden]);
}

}  // namespace
