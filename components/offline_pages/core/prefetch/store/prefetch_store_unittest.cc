// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/store/prefetch_store.h"

#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/offline_store_utils.h"
#include "components/offline_pages/core/prefetch/mock_prefetch_item_generator.h"
#include "components/offline_pages/core/prefetch/prefetch_item.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/prefetch/store/prefetch_store_test_util.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

class PrefetchStoreTest : public testing::Test {
 public:
  PrefetchStoreTest();
  ~PrefetchStoreTest() override = default;

  void SetUp() override { store_test_util_.BuildStoreInMemory(); }

  void TearDown() override {
    store_test_util_.DeleteStore();
  }

  PrefetchStore* store() { return store_test_util_.store(); }

  PrefetchStoreTestUtil* store_util() { return &store_test_util_; }
  MockPrefetchItemGenerator* item_generator() { return &item_generator_; }
  base::TestMockTimeTaskRunner* task_runner() { return task_runner_.get(); }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  PrefetchStoreTestUtil store_test_util_;
  MockPrefetchItemGenerator item_generator_;
};

PrefetchStoreTest::PrefetchStoreTest()
    : task_runner_(new base::TestMockTimeTaskRunner),
      task_runner_handle_(task_runner_),
      store_test_util_(task_runner_) {}

TEST_F(PrefetchStoreTest, InitializeStore) {
  EXPECT_EQ(0, store_util()->CountPrefetchItems());
}

TEST_F(PrefetchStoreTest, WriteAndLoadOneItem) {
  // Create an item populated with unique, non-default values.
  PrefetchItem item1(
      item_generator()->CreateItem(PrefetchItemState::DOWNLOADED));
  item1.generate_bundle_attempts = 10;
  item1.get_operation_attempts = 11;
  item1.download_initiation_attempts = 12;
  item1.creation_time = store_utils::FromDatabaseTime(1000L);
  item1.freshness_time = store_utils::FromDatabaseTime(2000L);
  item1.error_code = PrefetchItemErrorCode::TOO_MANY_NEW_URLS;
  item1.file_size = item1.archive_body_length + 1;

  EXPECT_TRUE(store_util()->InsertPrefetchItem(item1));
  std::set<PrefetchItem> all_items;
  EXPECT_EQ(1U, store_util()->GetAllItems(&all_items));
  EXPECT_EQ(1U, all_items.count(item1));
}

TEST_F(PrefetchStoreTest, ZombifyTestUtilWorks) {
  PrefetchItem item1(
      item_generator()->CreateItem(PrefetchItemState::NEW_REQUEST));
  EXPECT_EQ(0, store_util()->ZombifyPrefetchItems(item1.client_id.name_space,
                                                  item1.url));
  store_util()->InsertPrefetchItem(item1);
  EXPECT_EQ(1, store_util()->ZombifyPrefetchItems(item1.client_id.name_space,
                                                  item1.url));
  EXPECT_EQ(PrefetchItemState::ZOMBIE,
            store_util()->GetPrefetchItem(item1.offline_id)->state);
  EXPECT_EQ(1, store_util()->ZombifyPrefetchItems(item1.client_id.name_space,
                                                  item1.url));
}

TEST_F(PrefetchStoreTest, CloseStore) {
  // This ensures store is started.
  PrefetchItem item1(
      item_generator()->CreateItem(PrefetchItemState::NEW_REQUEST));
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item1));
  EXPECT_EQ(InitializationStatus::SUCCESS, store()->initialization_status());

  task_runner()->FastForwardBy(PrefetchStore::kClosingDelay);
  EXPECT_EQ(InitializationStatus::NOT_INITIALIZED,
            store()->initialization_status());

  // Should initialize the store again.
  PrefetchItem item2(
      item_generator()->CreateItem(PrefetchItemState::NEW_REQUEST));
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item2));
  EXPECT_EQ(InitializationStatus::SUCCESS, store()->initialization_status());
}

TEST_F(PrefetchStoreTest, CloseStorePostponed) {
  // This ensures store is started.
  PrefetchItem item1(
      item_generator()->CreateItem(PrefetchItemState::NEW_REQUEST));
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item1));
  EXPECT_EQ(InitializationStatus::SUCCESS, store()->initialization_status());

  task_runner()->FastForwardBy(PrefetchStore::kClosingDelay / 2);
  EXPECT_EQ(InitializationStatus::SUCCESS, store()->initialization_status());

  // Should postpone closing.
  PrefetchItem item2(
      item_generator()->CreateItem(PrefetchItemState::NEW_REQUEST));
  EXPECT_TRUE(store_util()->InsertPrefetchItem(item2));
  EXPECT_EQ(InitializationStatus::SUCCESS, store()->initialization_status());

  // This adds up to more than kClosingDelay after the first call, which means
  // the closing would trigger, it does not however, since second call caused it
  // to be postponed.
  task_runner()->FastForwardBy(2 * PrefetchStore::kClosingDelay / 3);
  // Store should still be initialized.
  EXPECT_EQ(InitializationStatus::SUCCESS, store()->initialization_status());
  // There is still a pending task to close the store.
  EXPECT_TRUE(task_runner()->HasPendingTask());

  // After this step the store should be closed.
  task_runner()->FastForwardBy(PrefetchStore::kClosingDelay);
  EXPECT_EQ(InitializationStatus::NOT_INITIALIZED,
            store()->initialization_status());
}

}  // namespace offline_pages
