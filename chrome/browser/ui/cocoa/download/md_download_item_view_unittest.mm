// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/md_download_item_view_testing.h"

#include "base/files/file_path.h"
#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/download/download_item_model.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "components/download/public/common/mock_download_item.h"
#include "testing/gtest_mac.h"

namespace {

class MDDownloadItemViewTest : public ui::CocoaTest {
 public:
  MDDownloadItemViewTest() {
    base::scoped_nsobject<MDDownloadItemView> view(
        [[MDDownloadItemView alloc] init]);
    view_ = view;
    [[test_window() contentView] addSubview:view_];

    ON_CALL(item_, GetFullPath())
        .WillByDefault(testing::ReturnRefOfCopy(base::FilePath("foo.bar")));
    ON_CALL(item_, GetFileNameToReportUser())
        .WillByDefault(testing::Return(base::FilePath("foo.bar")));
    [view_ setStateFromDownload:&model_];
  }

 protected:
  testing::NiceMock<download::MockDownloadItem> item_;
  DownloadItemModel model_{&item_};
  MDDownloadItemView* view_;  // Weak, owned by test_window().

  void set_state_and_display() {
    [view_ setStateFromDownload:&model_];
    [test_window() display];
  }
};

TEST_VIEW(MDDownloadItemViewTest, view_)

// Run the download item through a few states, including a danger state, mostly
// to prod for crashes. It isn't intended to cover every possible state.
TEST_F(MDDownloadItemViewTest, TestStates) {
  ON_CALL(item_, GetState())
      .WillByDefault(testing::Return(download::DownloadItem::IN_PROGRESS));
  set_state_and_display();
  EXPECT_NSEQ(nil, view_.dangerView);

  ON_CALL(item_, GetDangerType())
      .WillByDefault(
          testing::Return(download::DOWNLOAD_DANGER_TYPE_DANGEROUS_URL));
  ON_CALL(item_, IsDangerous()).WillByDefault(testing::Return(true));
  set_state_and_display();
  EXPECT_NSNE(nil, view_.dangerView);

  ON_CALL(item_, IsDangerous()).WillByDefault(testing::Return(false));
  set_state_and_display();
  EXPECT_NSEQ(nil, view_.dangerView);

  ON_CALL(item_, GetState())
      .WillByDefault(testing::Return(download::DownloadItem::COMPLETE));
  set_state_and_display();
}

// Verify that the key view loop is empty when full keyboard access is off and
// comprises the controls when it is on.
TEST_F(MDDownloadItemViewTest, TestKeyboardAccess) {
  EXPECT_NSEQ(test_window().validKeyViews, (@[
                // Nothing.
              ]));

  test_window().pretendFullKeyboardAccessIsEnabled = YES;

  EXPECT_NSEQ(test_window().validKeyViews, (@[
                view_.primaryButton,
                view_.menuButton,
              ]));
}

}  // namespace
