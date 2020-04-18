// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/md_downloads/downloads_list_tracker.h"

#include <limits.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/test/base/testing_profile.h"
#include "components/download/public/common/mock_download_item.h"
#include "content/public/test/mock_download_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_web_ui.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using download::DownloadItem;
using download::MockDownloadItem;
using DownloadVector = std::vector<DownloadItem*>;
using testing::_;
using testing::Return;

namespace {

uint64_t GetId(const base::Value& value) {
  const base::DictionaryValue* dict;
  CHECK(value.GetAsDictionary(&dict));

  int id;
  CHECK(dict->GetInteger("id", &id));
  CHECK_GE(id, 0);
  return static_cast<uint64_t>(id);
}

std::vector<uint64_t> GetIds(const base::Value& value) {
  std::vector<uint64_t> ids;

  const base::ListValue* list;
  if (value.GetAsList(&list)) {
    for (const auto& list_item : *list)
      ids.push_back(GetId(list_item));
  } else {
    ids.push_back(GetId(value));
  }

  return ids;
}

int GetIndex(const base::Value* value) {
  CHECK(value);
  int index;
  CHECK(value->GetAsInteger(&index));
  return index;
}

bool ShouldShowItem(const DownloadItem& item) {
  DownloadItemModel model(const_cast<DownloadItem*>(&item));
  return model.ShouldShowInShelf();
}

}  // namespace

// A test version of DownloadsListTracker.
class TestDownloadsListTracker : public DownloadsListTracker {
 public:
  TestDownloadsListTracker(content::DownloadManager* manager,
                           content::WebUI* web_ui)
      : DownloadsListTracker(manager, web_ui, base::Bind(&ShouldShowItem)) {}
  ~TestDownloadsListTracker() override {}

  using DownloadsListTracker::IsIncognito;
  using DownloadsListTracker::GetItemForTesting;
  using DownloadsListTracker::SetChunkSizeForTesting;

 protected:
  std::unique_ptr<base::DictionaryValue> CreateDownloadItemValue(
      download::DownloadItem* item) const override {
    std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
    CHECK_LE(item->GetId(), static_cast<uint64_t>(INT_MAX));
    dict->SetInteger("id", item->GetId());
    return dict;
  }
};

// A fixture to test DownloadsListTracker.
class DownloadsListTrackerTest : public testing::Test {
 public:
  DownloadsListTrackerTest() {}

  ~DownloadsListTrackerTest() override {
    for (const auto& mock_item : mock_items_)
      testing::Mock::VerifyAndClear(mock_item.get());
  }

  // testing::Test:
  void SetUp() override {
    ON_CALL(manager_, GetBrowserContext()).WillByDefault(Return(&profile_));
    ON_CALL(manager_, GetAllDownloads(_)).WillByDefault(
        testing::Invoke(this, &DownloadsListTrackerTest::GetAllDownloads));
  }

  MockDownloadItem* CreateMock(uint64_t id, const base::Time& started) {
    MockDownloadItem* new_item = new testing::NiceMock<MockDownloadItem>();
    mock_items_.push_back(base::WrapUnique(new_item));

    ON_CALL(*new_item, GetId()).WillByDefault(Return(id));
    ON_CALL(*new_item, GetStartTime()).WillByDefault(Return(started));
    ON_CALL(*new_item, IsTransient()).WillByDefault(Return(false));

    return new_item;
  }

  MockDownloadItem* CreateNextItem() {
    return CreateMock(mock_items_.size(), base::Time::UnixEpoch() +
        base::TimeDelta::FromHours(mock_items_.size()));
  }

  void CreateTracker() {
    tracker_.reset(new TestDownloadsListTracker(manager(), web_ui()));
  }

  TestingProfile* profile() { return &profile_; }
  content::DownloadManager* manager() { return &manager_; }
  content::TestWebUI* web_ui() { return &web_ui_; }
  TestDownloadsListTracker* tracker() { return tracker_.get(); }

 private:
  void GetAllDownloads(DownloadVector* result) {
    for (const auto& mock_item : mock_items_)
      result->push_back(mock_item.get());
  }

  // NOTE: The initialization order of these members matters.
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;

  testing::NiceMock<content::MockDownloadManager> manager_;
  content::TestWebUI web_ui_;
  std::unique_ptr<TestDownloadsListTracker> tracker_;

  std::vector<std::unique_ptr<MockDownloadItem>> mock_items_;
};

TEST_F(DownloadsListTrackerTest, SetSearchTerms) {
  CreateTracker();

  const base::ListValue empty_terms;
  EXPECT_FALSE(tracker()->SetSearchTerms(empty_terms));

  base::ListValue search_terms;
  search_terms.AppendString("search");
  EXPECT_TRUE(tracker()->SetSearchTerms(search_terms));

  EXPECT_FALSE(tracker()->SetSearchTerms(search_terms));

  EXPECT_TRUE(tracker()->SetSearchTerms(empty_terms));

  // Notifying the page is left up to the handler in this case.
  EXPECT_TRUE(web_ui()->call_data().empty());
}

TEST_F(DownloadsListTrackerTest, StartCallsInsertItems) {
  DownloadItem* first_item = CreateNextItem();

  CreateTracker();
  ASSERT_TRUE(tracker()->GetItemForTesting(0));
  EXPECT_TRUE(web_ui()->call_data().empty());

  tracker()->StartAndSendChunk();
  ASSERT_FALSE(web_ui()->call_data().empty());

  EXPECT_EQ("downloads.Manager.insertItems",
            web_ui()->call_data()[0]->function_name());
  EXPECT_EQ(0, GetIndex(web_ui()->call_data()[0]->arg1()));

  std::vector<uint64_t> ids = GetIds(*web_ui()->call_data()[0]->arg2());
  ASSERT_FALSE(ids.empty());
  EXPECT_EQ(first_item->GetId(), ids[0]);
}

// The page is in a loading state until it gets an insertItems call. Ensure that
// happens even without downloads.
TEST_F(DownloadsListTrackerTest, EmptyGetAllItemsStillCallsInsertItems) {
  CreateTracker();

  ASSERT_FALSE(tracker()->GetItemForTesting(0));
  ASSERT_TRUE(web_ui()->call_data().empty());

  tracker()->StartAndSendChunk();

  ASSERT_FALSE(web_ui()->call_data().empty());
  EXPECT_EQ("downloads.Manager.insertItems",
            web_ui()->call_data()[0]->function_name());
  ASSERT_TRUE(web_ui()->call_data()[0]->arg2());
  EXPECT_TRUE(GetIds(*web_ui()->call_data()[0]->arg2()).empty());
}

TEST_F(DownloadsListTrackerTest, OnDownloadCreatedCallsInsertItems) {
  CreateTracker();
  tracker()->StartAndSendChunk();
  web_ui()->ClearTrackedCalls();

  ASSERT_FALSE(tracker()->GetItemForTesting(0));
  DownloadItem* first_item = CreateNextItem();
  tracker()->OnDownloadCreated(manager(), first_item);

  ASSERT_FALSE(web_ui()->call_data().empty());
  EXPECT_EQ("downloads.Manager.insertItems",
            web_ui()->call_data()[0]->function_name());
  EXPECT_EQ(0, GetIndex(web_ui()->call_data()[0]->arg1()));

  std::vector<uint64_t> ids = GetIds(*web_ui()->call_data()[0]->arg2());
  ASSERT_FALSE(ids.empty());
  EXPECT_EQ(first_item->GetId(), ids[0]);
}

TEST_F(DownloadsListTrackerTest, OnDownloadRemovedCallsRemoveItem) {
  DownloadItem* first_item = CreateNextItem();

  CreateTracker();
  tracker()->StartAndSendChunk();
  web_ui()->ClearTrackedCalls();

  EXPECT_TRUE(tracker()->GetItemForTesting(0));
  tracker()->OnDownloadRemoved(manager(), first_item);
  EXPECT_FALSE(tracker()->GetItemForTesting(0));

  ASSERT_FALSE(web_ui()->call_data().empty());

  EXPECT_EQ("downloads.Manager.removeItem",
            web_ui()->call_data()[0]->function_name());
  EXPECT_EQ(0, GetIndex(web_ui()->call_data()[0]->arg1()));
}

TEST_F(DownloadsListTrackerTest, OnDownloadUpdatedCallsRemoveItem) {
  DownloadItem* first_item = CreateNextItem();

  CreateTracker();
  tracker()->StartAndSendChunk();
  web_ui()->ClearTrackedCalls();

  EXPECT_TRUE(tracker()->GetItemForTesting(0));

  DownloadItemModel(first_item).SetShouldShowInShelf(false);
  tracker()->OnDownloadUpdated(manager(), first_item);

  EXPECT_FALSE(tracker()->GetItemForTesting(0));

  ASSERT_FALSE(web_ui()->call_data().empty());

  EXPECT_EQ("downloads.Manager.removeItem",
            web_ui()->call_data()[0]->function_name());
  EXPECT_EQ(0, GetIndex(web_ui()->call_data()[0]->arg1()));
}

TEST_F(DownloadsListTrackerTest, StartExcludesHiddenItems) {
  DownloadItem* first_item = CreateNextItem();
  DownloadItemModel(first_item).SetShouldShowInShelf(false);

  CreateTracker();
  tracker()->StartAndSendChunk();

  ASSERT_FALSE(web_ui()->call_data().empty());

  EXPECT_EQ("downloads.Manager.insertItems",
            web_ui()->call_data()[0]->function_name());
  EXPECT_TRUE(GetIds(*web_ui()->call_data()[0]->arg2()).empty());
}

TEST_F(DownloadsListTrackerTest, Incognito) {
  testing::NiceMock<content::MockDownloadManager> incognito_manager;
  ON_CALL(incognito_manager, GetBrowserContext()).WillByDefault(Return(
      TestingProfile::Builder().BuildIncognito(profile())));

  MockDownloadItem item;
  EXPECT_CALL(item, GetId()).WillRepeatedly(Return(0));

  ON_CALL(incognito_manager, GetDownload(0)).WillByDefault(Return(&item));

  TestDownloadsListTracker tracker(&incognito_manager, web_ui());
  EXPECT_TRUE(tracker.IsIncognito(item));
}

TEST_F(DownloadsListTrackerTest, OnlySendSomeItems) {
  CreateNextItem();
  CreateNextItem();
  CreateNextItem();
  CreateNextItem();
  CreateNextItem();

  CreateTracker();
  tracker()->SetChunkSizeForTesting(3);
  tracker()->StartAndSendChunk();

  ASSERT_FALSE(web_ui()->call_data().empty());

  EXPECT_EQ("downloads.Manager.insertItems",
            web_ui()->call_data()[0]->function_name());
  EXPECT_EQ(0, GetIndex(web_ui()->call_data()[0]->arg1()));
  EXPECT_EQ(3u, GetIds(*web_ui()->call_data()[0]->arg2()).size());

  tracker()->StartAndSendChunk();
  ASSERT_GE(2u, web_ui()->call_data().size());

  EXPECT_EQ("downloads.Manager.insertItems",
            web_ui()->call_data()[1]->function_name());
  EXPECT_EQ(3, GetIndex(web_ui()->call_data()[1]->arg1()));
  EXPECT_EQ(2u, GetIds(*web_ui()->call_data()[1]->arg2()).size());
}

TEST_F(DownloadsListTrackerTest, IgnoreUnsentItemUpdates) {
  DownloadItem* unsent_item = CreateNextItem();
  CreateNextItem();

  CreateTracker();
  tracker()->SetChunkSizeForTesting(1);
  tracker()->StartAndSendChunk();

  ASSERT_FALSE(web_ui()->call_data().empty());

  EXPECT_EQ("downloads.Manager.insertItems",
            web_ui()->call_data()[0]->function_name());
  EXPECT_EQ(1u, GetIds(*web_ui()->call_data()[0]->arg2()).size());

  tracker()->OnDownloadUpdated(manager(), unsent_item);
  EXPECT_EQ(1u, web_ui()->call_data().size());
}

TEST_F(DownloadsListTrackerTest, IgnoreUnsentItemRemovals) {
  DownloadItem* unsent_item = CreateNextItem();
  CreateNextItem();

  CreateTracker();
  tracker()->SetChunkSizeForTesting(1);
  tracker()->StartAndSendChunk();

  ASSERT_FALSE(web_ui()->call_data().empty());

  EXPECT_EQ("downloads.Manager.insertItems",
            web_ui()->call_data()[0]->function_name());
  EXPECT_EQ(1u, GetIds(*web_ui()->call_data()[0]->arg2()).size());

  DownloadItemModel(unsent_item).SetShouldShowInShelf(false);
  tracker()->OnDownloadUpdated(manager(), unsent_item);
  EXPECT_EQ(1u, web_ui()->call_data().size());

  DownloadItemModel(unsent_item).SetShouldShowInShelf(true);
  tracker()->OnDownloadUpdated(manager(), unsent_item);
  EXPECT_EQ(1u, web_ui()->call_data().size());
}

TEST_F(DownloadsListTrackerTest, IgnoreTransientDownloads) {
  MockDownloadItem* transient_item = CreateNextItem();
  ON_CALL(*transient_item, IsTransient()).WillByDefault(Return(true));

  CreateTracker();
  tracker()->StartAndSendChunk();

  ASSERT_FALSE(web_ui()->call_data().empty());
  EXPECT_EQ(0u, GetIds(*web_ui()->call_data()[0]->arg2()).size());
}
