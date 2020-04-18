// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/add_page_to_download_manager_task.h"

#include <memory>

#include "components/offline_pages/core/model/model_task_test_base.h"
#include "components/offline_pages/core/stub_system_download_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kTitle[] = "testPageTitle";
const char kDescription[] = "test page description";
const char kPath[] = "/sdcard/Download/page.mhtml";
const char kUri[] = "https://www.google.com";
const char kReferer[] = "https://google.com";
const long kTestLength = 1024;
const long kTestDownloadId = 42;
const long kDefaultDownloadId = 1;
}  // namespace

namespace offline_pages {

class AddPageToDownloadManagerTaskTest : public ModelTaskTestBase {
 public:
  AddPageToDownloadManagerTaskTest();
  ~AddPageToDownloadManagerTaskTest() override;

  StubSystemDownloadManager* download_manager() {
    return download_manager_.get();
  }

 private:
  std::unique_ptr<StubSystemDownloadManager> download_manager_;
};

AddPageToDownloadManagerTaskTest::AddPageToDownloadManagerTaskTest()
    : download_manager_(new StubSystemDownloadManager(kTestDownloadId, true)) {}

AddPageToDownloadManagerTaskTest::~AddPageToDownloadManagerTaskTest() {}

TEST_F(AddPageToDownloadManagerTaskTest, AddSimpleId) {
  OfflinePageItem page = generator()->CreateItem();
  store_test_util()->InsertItem(page);

  RunTask(std::make_unique<AddPageToDownloadManagerTask>(
      store(), download_manager(), page.offline_id, kTitle, kDescription, kPath,
      kTestLength, kUri, kReferer));

  // Check the download ID got set in the offline page item in the database.
  auto offline_page = store_test_util()->GetPageByOfflineId(page.offline_id);
  ASSERT_TRUE(offline_page);
  EXPECT_EQ(offline_page->system_download_id, kTestDownloadId);

  // Check that the system download manager stub saw the arguments it expected
  EXPECT_EQ(download_manager()->title(), std::string(kTitle));
  EXPECT_EQ(download_manager()->description(), kDescription);
  EXPECT_EQ(download_manager()->path(), kPath);
  EXPECT_EQ(download_manager()->uri(), kUri);
  EXPECT_EQ(download_manager()->referer(), kReferer);
  EXPECT_EQ(download_manager()->length(), kTestLength);
}

TEST_F(AddPageToDownloadManagerTaskTest, NoADM) {
  // Simulate the ADM being unavailable on the system.
  download_manager()->set_installed(false);

  OfflinePageItem page = generator()->CreateItem();
  store_test_util()->InsertItem(page);

  RunTask(std::make_unique<AddPageToDownloadManagerTask>(
      store(), download_manager(), page.offline_id, kTitle, kDescription, kPath,
      kTestLength, kUri, kReferer));

  // Check the download ID did not get set in the offline page item in the
  // database.
  auto offline_page = store_test_util()->GetPageByOfflineId(page.offline_id);
  ASSERT_TRUE(offline_page);
  EXPECT_EQ(offline_page->system_download_id, 0);
}

TEST_F(AddPageToDownloadManagerTaskTest, AddDownloadFailed) {
  // Simulate failure by asking the download manager to return id of 0.
  download_manager()->set_download_id(0);
  OfflinePageItem page = generator()->CreateItem();
  page.system_download_id = kDefaultDownloadId;
  store_test_util()->InsertItem(page);

  RunTask(std::make_unique<AddPageToDownloadManagerTask>(
      store(), download_manager(), page.offline_id, kTitle, kDescription, kPath,
      kTestLength, kUri, kReferer));

  // Check the download ID did not get set in the offline page item in the
  // database.
  auto offline_page = store_test_util()->GetPageByOfflineId(page.offline_id);
  ASSERT_TRUE(offline_page);
  EXPECT_EQ(offline_page->system_download_id, kDefaultDownloadId);
}

}  // namespace offline_pages
