// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/start_offline_page_upgrade_task.h"

#include <memory>

#include "components/offline_pages/core/model/model_task_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
const char kTestDigest[] = "TestDigest==";
}  // namespace

class StartOfflinePageUpgradeTaskTest : public ModelTaskTestBase {
 public:
  StartOfflinePageUpgradeTaskTest();
  ~StartOfflinePageUpgradeTaskTest() override;

  void StartUpgradeDone(StartUpgradeResult result);

  StartUpgradeCallback callback() {
    return base::BindOnce(&StartOfflinePageUpgradeTaskTest::StartUpgradeDone,
                          base::Unretained(this));
  }

  StartUpgradeResult* last_result() { return &last_result_; }

 private:
  StartUpgradeResult last_result_;
};

StartOfflinePageUpgradeTaskTest::StartOfflinePageUpgradeTaskTest()
    : last_result_(StartUpgradeStatus::DB_ERROR) {}

StartOfflinePageUpgradeTaskTest::~StartOfflinePageUpgradeTaskTest() {}

void StartOfflinePageUpgradeTaskTest::StartUpgradeDone(
    StartUpgradeResult result) {
  last_result_ = std::move(result);
}

TEST_F(StartOfflinePageUpgradeTaskTest, StartUpgradeSuccess) {
  OfflinePageItem original_page = generator()->CreateItemWithTempFile();
  original_page.upgrade_attempt = 3;
  original_page.digest = kTestDigest;
  store_test_util()->InsertItem(original_page);

  auto task = std::make_unique<StartOfflinePageUpgradeTask>(
      store(), original_page.offline_id, TemporaryDir(), callback());
  RunTask(std::move(task));

  EXPECT_EQ(StartUpgradeStatus::SUCCESS, last_result()->status);
  EXPECT_EQ(kTestDigest, last_result()->digest);
  EXPECT_EQ(original_page.file_path, last_result()->file_path);

  auto upgraded_page =
      store_test_util()->GetPageByOfflineId(original_page.offline_id);
  ASSERT_TRUE(upgraded_page);
  EXPECT_EQ(2, upgraded_page->upgrade_attempt);
}

TEST_F(StartOfflinePageUpgradeTaskTest, StartUpgradeItemMissing) {
  auto task = std::make_unique<StartOfflinePageUpgradeTask>(
      store(), 42, base::FilePath(), callback());
  RunTask(std::move(task));

  EXPECT_EQ(StartUpgradeStatus::ITEM_MISSING, last_result()->status);
  EXPECT_TRUE(last_result()->digest.empty());
  EXPECT_TRUE(last_result()->file_path.empty());
}

TEST_F(StartOfflinePageUpgradeTaskTest, StartUpgradeFileMissing) {
  OfflinePageItem original_page = generator()->CreateItem();
  original_page.upgrade_attempt = 3;
  store_test_util()->InsertItem(original_page);

  auto task = std::make_unique<StartOfflinePageUpgradeTask>(
      store(), original_page.offline_id, base::FilePath(), callback());
  RunTask(std::move(task));

  EXPECT_EQ(StartUpgradeStatus::FILE_MISSING, last_result()->status);
  EXPECT_TRUE(last_result()->digest.empty());
  EXPECT_TRUE(last_result()->file_path.empty());

  auto upgraded_page =
      store_test_util()->GetPageByOfflineId(original_page.offline_id);
  ASSERT_TRUE(upgraded_page);
  EXPECT_EQ(original_page, *upgraded_page);
}

TEST_F(StartOfflinePageUpgradeTaskTest, StartUpgradeNotEnoughSpace) {
  OfflinePageItem original_page = generator()->CreateItemWithTempFile();
  original_page.upgrade_attempt = 3;
  store_test_util()->InsertItem(original_page);

  auto task = std::make_unique<StartOfflinePageUpgradeTask>(
      store(), original_page.offline_id, base::FilePath(), callback());
  RunTask(std::move(task));

  EXPECT_EQ(StartUpgradeStatus::NOT_ENOUGH_STORAGE, last_result()->status);
  EXPECT_TRUE(last_result()->digest.empty());
  EXPECT_TRUE(last_result()->file_path.empty());

  auto upgraded_page =
      store_test_util()->GetPageByOfflineId(original_page.offline_id);
  ASSERT_TRUE(upgraded_page);
  EXPECT_EQ(original_page, *upgraded_page);
}

}  // namespace offline_pages
