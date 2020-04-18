// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/model/get_pages_task.h"

#include <stdint.h>

#include <utility>

#include "components/offline_pages/core/client_namespace_constants.h"
#include "components/offline_pages/core/model/model_task_test_base.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {

const char kTestNamespace[] = "test_namespace";

}  // namespace

class GetPagesTaskTest : public ModelTaskTestBase {
 public:
  void OnGetPagesDone(const std::vector<OfflinePageItem>& pages);
  void OnGetPageDone(const OfflinePageItem* page);

  MultipleOfflinePageItemCallback get_pages_callback();
  SingleOfflinePageItemCallback get_single_page_callback();

  const std::vector<OfflinePageItem>& read_result() const {
    return read_result_;
  }

  const OfflinePageItem& single_page_result() const {
    return single_page_result_;
  }

 private:
  std::vector<OfflinePageItem> read_result_;
  OfflinePageItem single_page_result_;
};

void GetPagesTaskTest::OnGetPagesDone(
    const std::vector<OfflinePageItem>& result) {
  read_result_.clear();
  for (auto page : result)
    read_result_.push_back(page);
}

void GetPagesTaskTest::OnGetPageDone(const OfflinePageItem* page) {
  if (!page)
    return;
  single_page_result_ = *page;
}

MultipleOfflinePageItemCallback GetPagesTaskTest::get_pages_callback() {
  return base::BindOnce(&GetPagesTaskTest::OnGetPagesDone,
                        base::Unretained(this));
}

SingleOfflinePageItemCallback GetPagesTaskTest::get_single_page_callback() {
  return base::BindOnce(&GetPagesTaskTest::OnGetPageDone,
                        base::Unretained(this));
}

TEST_F(GetPagesTaskTest, GetAllPages) {
  generator()->SetNamespace(kTestNamespace);
  OfflinePageItem item_1 = generator()->CreateItem();
  store_test_util()->InsertItem(item_1);
  OfflinePageItem item_2 = generator()->CreateItem();
  store_test_util()->InsertItem(item_2);
  OfflinePageItem item_3 = generator()->CreateItem();
  store_test_util()->InsertItem(item_3);

  RunTask(
      GetPagesTask::CreateTaskMatchingAllPages(store(), get_pages_callback()));

  std::set<OfflinePageItem> result_set;
  result_set.insert(read_result().begin(), read_result().end());
  EXPECT_EQ(3UL, result_set.size());
  EXPECT_EQ(1UL, result_set.count(item_1));
  EXPECT_EQ(1UL, result_set.count(item_2));
  EXPECT_EQ(1UL, result_set.count(item_3));
}

TEST_F(GetPagesTaskTest, GetPagesForSingleClientId) {
  generator()->SetNamespace(kTestNamespace);
  OfflinePageItem item_1 = generator()->CreateItem();
  store_test_util()->InsertItem(item_1);

  std::vector<ClientId> client_ids = {item_1.client_id};

  RunTask(GetPagesTask::CreateTaskMatchingClientIds(
      store(), get_pages_callback(), client_ids));

  EXPECT_EQ(1UL, read_result().size());
  EXPECT_EQ(item_1, *(read_result().begin()));
}

TEST_F(GetPagesTaskTest, GetPagesForMultipleClientIds) {
  generator()->SetNamespace(kTestNamespace);
  OfflinePageItem item_1 = generator()->CreateItem();
  store_test_util()->InsertItem(item_1);
  OfflinePageItem item_2 = generator()->CreateItem();
  store_test_util()->InsertItem(item_2);
  OfflinePageItem item_3 = generator()->CreateItem();
  store_test_util()->InsertItem(item_3);

  std::vector<ClientId> client_ids{item_1.client_id, item_2.client_id};

  RunTask(GetPagesTask::CreateTaskMatchingClientIds(
      store(), get_pages_callback(), client_ids));

  std::set<OfflinePageItem> result_set;
  result_set.insert(read_result().begin(), read_result().end());
  EXPECT_EQ(2UL, result_set.size());
  EXPECT_EQ(1UL, result_set.count(item_1));
  EXPECT_EQ(1UL, result_set.count(item_2));
}

TEST_F(GetPagesTaskTest, GetPagesByNamespace) {
  static const char kOtherNamespace[] = "other_namespace";
  generator()->SetNamespace(kTestNamespace);
  OfflinePageItem item_1 = generator()->CreateItem();
  store_test_util()->InsertItem(item_1);
  OfflinePageItem item_2 = generator()->CreateItem();
  store_test_util()->InsertItem(item_2);
  generator()->SetNamespace(kOtherNamespace);
  OfflinePageItem item_3 = generator()->CreateItem();
  store_test_util()->InsertItem(item_3);

  RunTask(GetPagesTask::CreateTaskMatchingNamespace(
      store(), get_pages_callback(), kTestNamespace));

  std::set<OfflinePageItem> result_set;
  result_set.insert(read_result().begin(), read_result().end());
  EXPECT_EQ(2UL, result_set.size());
  EXPECT_EQ(1UL, result_set.count(item_1));
  EXPECT_EQ(1UL, result_set.count(item_2));
}

TEST_F(GetPagesTaskTest, GetPagesByRequestOrigin) {
  static const char kRequestOrigin1[] = "bar";
  static const char kRequestOrigin2[] = "baz";
  generator()->SetNamespace(kTestNamespace);
  generator()->SetRequestOrigin(kRequestOrigin1);
  OfflinePageItem item_1 = generator()->CreateItem();
  store_test_util()->InsertItem(item_1);
  OfflinePageItem item_2 = generator()->CreateItem();
  store_test_util()->InsertItem(item_2);
  generator()->SetRequestOrigin(kRequestOrigin2);
  OfflinePageItem item_3 = generator()->CreateItem();
  store_test_util()->InsertItem(item_3);

  RunTask(GetPagesTask::CreateTaskMatchingRequestOrigin(
      store(), get_pages_callback(), kRequestOrigin1));

  std::set<OfflinePageItem> result_set;
  result_set.insert(read_result().begin(), read_result().end());
  EXPECT_EQ(2UL, result_set.size());
  EXPECT_EQ(1UL, result_set.count(item_1));
  EXPECT_EQ(1UL, result_set.count(item_2));
}

TEST_F(GetPagesTaskTest, GetPagesByUrl) {
  static const GURL kUrl1("http://cs.chromium.org");
  static const GURL kUrl1Frag("http://cs.chromium.org#frag1");
  static const GURL kUrl2("http://chrome.google.com");
  generator()->SetNamespace(kTestNamespace);
  generator()->SetUrl(kUrl1);
  OfflinePageItem item_1 = generator()->CreateItem();
  store_test_util()->InsertItem(item_1);

  generator()->SetUrl(kUrl1Frag);
  OfflinePageItem item_2 = generator()->CreateItem();
  store_test_util()->InsertItem(item_2);

  generator()->SetUrl(kUrl2);
  generator()->SetOriginalUrl(kUrl1);
  OfflinePageItem item_3 = generator()->CreateItem();
  store_test_util()->InsertItem(item_3);

  generator()->SetUrl(kUrl2);
  generator()->SetOriginalUrl(kUrl1Frag);
  OfflinePageItem item_4 = generator()->CreateItem();
  store_test_util()->InsertItem(item_4);

  generator()->SetUrl(kUrl2);
  generator()->SetOriginalUrl(kUrl2);
  OfflinePageItem item_5 = generator()->CreateItem();
  store_test_util()->InsertItem(item_5);

  RunTask(GetPagesTask::CreateTaskMatchingUrl(store(), get_pages_callback(),
                                              kUrl1));

  std::set<OfflinePageItem> result_set;
  result_set.insert(read_result().begin(), read_result().end());
  EXPECT_EQ(4UL, result_set.size());
  EXPECT_EQ(1UL, result_set.count(item_1));
  EXPECT_EQ(1UL, result_set.count(item_2));
  EXPECT_EQ(1UL, result_set.count(item_3));
  EXPECT_EQ(1UL, result_set.count(item_4));
}

TEST_F(GetPagesTaskTest, GetPageByOfflineId) {
  generator()->SetNamespace(kTestNamespace);
  OfflinePageItem item_1 = generator()->CreateItem();
  store_test_util()->InsertItem(item_1);
  OfflinePageItem item_2 = generator()->CreateItem();
  store_test_util()->InsertItem(item_2);
  OfflinePageItem item_3 = generator()->CreateItem();
  store_test_util()->InsertItem(item_3);

  RunTask(GetPagesTask::CreateTaskMatchingOfflineId(
      store(), get_single_page_callback(), item_1.offline_id));

  EXPECT_EQ(item_1, single_page_result());
}

TEST_F(GetPagesTaskTest, GetPageByGuid) {
  OfflinePageItem item_1 = generator()->CreateItem();
  store_test_util()->InsertItem(item_1);
  OfflinePageItem item_2 = generator()->CreateItem();
  store_test_util()->InsertItem(item_2);
  OfflinePageItem item_3 = generator()->CreateItem();
  store_test_util()->InsertItem(item_3);

  RunTask(GetPagesTask::CreateTaskMatchingGuid(
      store(), get_single_page_callback(), item_1.client_id.id));

  EXPECT_EQ(item_1, single_page_result());
}

TEST_F(GetPagesTaskTest, GetPageBySizeAndDigest) {
  static const int64_t kFileSize1 = 123LL;
  static const int64_t kFileSize2 = 999999LL;
  static const char kDigest1[] = "digest 1";
  static const char kDigest2[] = "digest 2";
  generator()->SetFileSize(kFileSize1);
  generator()->SetDigest(kDigest1);
  OfflinePageItem item_1 = generator()->CreateItem();
  store_test_util()->InsertItem(item_1);
  generator()->SetDigest(kDigest2);
  OfflinePageItem item_2 = generator()->CreateItem();
  store_test_util()->InsertItem(item_2);
  generator()->SetFileSize(kFileSize2);
  OfflinePageItem item_3 = generator()->CreateItem();
  store_test_util()->InsertItem(item_3);

  RunTask(GetPagesTask::CreateTaskMatchingSizeAndDigest(
      store(), get_single_page_callback(), kFileSize1, "mismatched digest"));
  EXPECT_EQ(OfflinePageItem(), single_page_result());

  RunTask(GetPagesTask::CreateTaskMatchingSizeAndDigest(
      store(), get_single_page_callback(), 0LL, kDigest1));
  EXPECT_EQ(OfflinePageItem(), single_page_result());

  RunTask(GetPagesTask::CreateTaskMatchingSizeAndDigest(
      store(), get_single_page_callback(), kFileSize2, kDigest2));
  EXPECT_EQ(item_3, single_page_result());
}

TEST_F(GetPagesTaskTest, GetPagesSupportedByDownloads) {
  generator()->SetNamespace(kCCTNamespace);
  store_test_util()->InsertItem(generator()->CreateItem());
  generator()->SetNamespace(kDownloadNamespace);
  OfflinePageItem download_item = generator()->CreateItem();
  store_test_util()->InsertItem(download_item);
  generator()->SetNamespace(kNTPSuggestionsNamespace);
  OfflinePageItem ntp_suggestion_item = generator()->CreateItem();
  store_test_util()->InsertItem(ntp_suggestion_item);

  RunTask(GetPagesTask::CreateTaskMatchingPagesSupportedByDownloads(
      store(), get_pages_callback(), policy_controller()));

  std::set<OfflinePageItem> result_set;
  result_set.insert(read_result().begin(), read_result().end());
  EXPECT_EQ(2UL, result_set.size());
  EXPECT_EQ(1UL, result_set.count(download_item));
  EXPECT_EQ(1UL, result_set.count(ntp_suggestion_item));
}

TEST_F(GetPagesTaskTest, GetPagesRemovedOnCacheReset) {
  generator()->SetNamespace(kCCTNamespace);
  OfflinePageItem cct_item = generator()->CreateItem();
  store_test_util()->InsertItem(cct_item);
  generator()->SetNamespace(kDownloadNamespace);
  store_test_util()->InsertItem(generator()->CreateItem());
  generator()->SetNamespace(kNTPSuggestionsNamespace);
  store_test_util()->InsertItem(generator()->CreateItem());

  RunTask(GetPagesTask::CreateTaskMatchingPagesRemovedOnCacheReset(
      store(), get_pages_callback(), policy_controller()));

  std::set<OfflinePageItem> result_set;
  result_set.insert(read_result().begin(), read_result().end());
  EXPECT_EQ(1UL, result_set.size());
  EXPECT_EQ(1UL, result_set.count(cct_item));
}

TEST_F(GetPagesTaskTest, SelectItemsForUpgrade) {
  base::Time now = base::Time::Now();
  std::vector<int> remaining_attempts = {3, 2, 2, 1};
  std::vector<base::Time> creation_times = {
      now, now, now - base::TimeDelta::FromDays(1), now};

  // |expected_items| are items expected to be selected by the task.
  std::vector<OfflinePageItem> expected_items;

  generator()->SetNamespace(kDownloadNamespace);
  for (size_t i = 0; i < remaining_attempts.size(); ++i) {
    OfflinePageItem selected_item = generator()->CreateItem();
    selected_item.upgrade_attempt = remaining_attempts[i];
    selected_item.creation_time = creation_times[i];
    store_test_util()->InsertItem(selected_item);
    // This selected_item is expected in return and in this position.
    expected_items.push_back(selected_item);

    // Should be skipped (no more upgrade attempts available).
    OfflinePageItem non_selected_item = generator()->CreateItem();
    non_selected_item.upgrade_attempt = 0;
    non_selected_item.creation_time = creation_times[i];
    store_test_util()->InsertItem(non_selected_item);
  }

  RunTask(GetPagesTask::CreateTaskSelectingItemsMarkedForUpgrade(
      store(), get_pages_callback()));

  ASSERT_TRUE(expected_items.size() == read_result().size());
  for (size_t i = 0; i < expected_items.size(); ++i)
    EXPECT_EQ(expected_items[i], read_result()[i]);
}

}  // namespace offline_pages
