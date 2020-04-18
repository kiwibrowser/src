// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/compiler_specific.h"
#include "base/run_loop.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/download/download_core_service_factory.h"
#include "chrome/browser/download/download_core_service_impl.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/browser/download/test_download_shelf.h"
#include "chrome/test/base/testing_profile.h"
#include "components/download/public/common/mock_download_item.h"
#include "content/public/browser/download_item_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/mock_download_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/common/extension.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Return;
using ::testing::ReturnRefOfCopy;
using ::testing::SaveArg;
using ::testing::_;
using download::DownloadItem;

namespace {

class DownloadShelfTest : public testing::Test {
 public:
  DownloadShelfTest();

 protected:
  download::MockDownloadItem* download_item() { return download_item_.get(); }
  content::MockDownloadManager* download_manager() {
    return download_manager_.get();
  }
  TestDownloadShelf* shelf() {
    return &shelf_;
  }
  Profile* profile() { return profile_.get(); }

  void SetUp() override {
  }

  void TearDown() override {
  }

 private:
  std::unique_ptr<download::MockDownloadItem> GetInProgressMockDownload();

  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  std::unique_ptr<download::MockDownloadItem> download_item_;
  std::unique_ptr<content::MockDownloadManager> download_manager_;
  TestDownloadShelf shelf_;
  std::unique_ptr<TestingProfile> profile_;
};

DownloadShelfTest::DownloadShelfTest() : profile_(new TestingProfile()) {
  download_item_.reset(new ::testing::NiceMock<download::MockDownloadItem>());
  ON_CALL(*download_item_, GetAutoOpened()).WillByDefault(Return(false));
  ON_CALL(*download_item_, GetMimeType()).WillByDefault(Return("text/plain"));
  ON_CALL(*download_item_, GetOpenWhenComplete()).WillByDefault(Return(false));
  ON_CALL(*download_item_, GetTargetDisposition())
      .WillByDefault(Return(DownloadItem::TARGET_DISPOSITION_OVERWRITE));
  ON_CALL(*download_item_, GetURL())
      .WillByDefault(ReturnRefOfCopy(GURL("http://example.com/foo")));
  ON_CALL(*download_item_, GetState())
      .WillByDefault(Return(DownloadItem::IN_PROGRESS));
  ON_CALL(*download_item_, IsTemporary()).WillByDefault(Return(false));
  ON_CALL(*download_item_, ShouldOpenFileBasedOnExtension())
      .WillByDefault(Return(false));
  content::DownloadItemUtils::AttachInfo(download_item_.get(), profile(),
                                         nullptr);

  download_manager_.reset(
      new ::testing::NiceMock<content::MockDownloadManager>());
  ON_CALL(*download_manager_, GetDownload(_))
      .WillByDefault(Return(download_item_.get()));
  ON_CALL(*download_manager_, GetBrowserContext())
      .WillByDefault(Return(profile()));

  shelf_.set_download_manager(download_manager_.get());
}

} // namespace

TEST_F(DownloadShelfTest, ClosesShelfWhenHidden) {
  shelf()->Open();
  EXPECT_TRUE(shelf()->IsShowing());
  shelf()->Hide();
  EXPECT_FALSE(shelf()->IsShowing());
  shelf()->Unhide();
  EXPECT_TRUE(shelf()->IsShowing());
}

TEST_F(DownloadShelfTest, CloseWhileHiddenPreventsShowOnUnhide) {
  shelf()->Open();
  shelf()->Hide();
  shelf()->Close(DownloadShelf::AUTOMATIC);
  shelf()->Unhide();
  EXPECT_FALSE(shelf()->IsShowing());
}

TEST_F(DownloadShelfTest, UnhideDoesntShowIfNotShownOnHide) {
  shelf()->Hide();
  shelf()->Unhide();
  EXPECT_FALSE(shelf()->IsShowing());
}

TEST_F(DownloadShelfTest, AddDownloadWhileHiddenUnhides) {
  shelf()->Open();
  shelf()->Hide();
  shelf()->AddDownload(download_item());
  EXPECT_TRUE(shelf()->IsShowing());
}

TEST_F(DownloadShelfTest, AddDownloadWhileHiddenUnhidesAndShows) {
  shelf()->Hide();
  shelf()->AddDownload(download_item());
  EXPECT_TRUE(shelf()->IsShowing());
}

// Normal downloads should be added synchronously and cause the shelf to show.
TEST_F(DownloadShelfTest, AddNormalDownload) {
  EXPECT_FALSE(shelf()->IsShowing());
  shelf()->AddDownload(download_item());
  EXPECT_TRUE(shelf()->did_add_download());
  EXPECT_TRUE(shelf()->IsShowing());
}

// Add a transient download. It should not be added immediately. Instead it
// should be added after a delay. For testing, the delay is set to 0 seconds. So
// the download should be added once the message loop is flushed.
TEST_F(DownloadShelfTest, AddDelayedDownload) {
  EXPECT_CALL(*download_item(), ShouldOpenFileBasedOnExtension())
      .WillRepeatedly(Return(true));
  ASSERT_TRUE(DownloadItemModel(download_item())
              .ShouldRemoveFromShelfWhenComplete());
  shelf()->AddDownload(download_item());

  EXPECT_FALSE(shelf()->did_add_download());
  EXPECT_FALSE(shelf()->IsShowing());

  base::RunLoop run_loop;
  run_loop.RunUntilIdle();

  EXPECT_TRUE(shelf()->did_add_download());
  EXPECT_TRUE(shelf()->IsShowing());
}

// Add a transient download that completes before the delay. It should not be
// displayed on the shelf.
TEST_F(DownloadShelfTest, AddDelayedCompletedDownload) {
  EXPECT_CALL(*download_item(), ShouldOpenFileBasedOnExtension())
      .WillRepeatedly(Return(true));
  ASSERT_TRUE(DownloadItemModel(download_item())
              .ShouldRemoveFromShelfWhenComplete());
  shelf()->AddDownload(download_item());

  EXPECT_FALSE(shelf()->did_add_download());
  EXPECT_FALSE(shelf()->IsShowing());

  EXPECT_CALL(*download_item(), GetState())
      .WillRepeatedly(Return(DownloadItem::COMPLETE));
  EXPECT_CALL(*download_item(), GetAutoOpened())
      .WillRepeatedly(Return(true));

  base::RunLoop run_loop;
  run_loop.RunUntilIdle();

  EXPECT_FALSE(shelf()->did_add_download());
  EXPECT_FALSE(shelf()->IsShowing());
}

// Add a transient download that completes and becomes non-transient before the
// delay. It should be displayed on the shelf even though it is complete.
TEST_F(DownloadShelfTest, AddDelayedCompleteNonTransientDownload) {
  EXPECT_CALL(*download_item(), ShouldOpenFileBasedOnExtension())
      .WillRepeatedly(Return(true));
  ASSERT_TRUE(DownloadItemModel(download_item())
              .ShouldRemoveFromShelfWhenComplete());
  shelf()->AddDownload(download_item());

  EXPECT_FALSE(shelf()->did_add_download());
  EXPECT_FALSE(shelf()->IsShowing());

  EXPECT_CALL(*download_item(), GetState())
      .WillRepeatedly(Return(DownloadItem::COMPLETE));
  EXPECT_CALL(*download_item(), ShouldOpenFileBasedOnExtension())
      .WillRepeatedly(Return(false));
  ASSERT_FALSE(DownloadItemModel(download_item())
               .ShouldRemoveFromShelfWhenComplete());

  base::RunLoop run_loop;
  run_loop.RunUntilIdle();

  EXPECT_TRUE(shelf()->did_add_download());
  EXPECT_TRUE(shelf()->IsShowing());
}
