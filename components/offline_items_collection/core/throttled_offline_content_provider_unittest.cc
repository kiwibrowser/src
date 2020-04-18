// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_items_collection/core/offline_content_aggregator.h"

#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/offline_items_collection/core/offline_item.h"
#include "components/offline_items_collection/core/test_support/mock_offline_content_provider.h"
#include "components/offline_items_collection/core/test_support/scoped_mock_offline_content_provider.h"
#include "components/offline_items_collection/core/throttled_offline_content_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gmock_mutant.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::CallbackToFunctor;
using testing::InvokeWithoutArgs;
using testing::Return;

namespace offline_items_collection {
namespace {

// Helper class to automatically trigger another OnItemUpdated() to the
// underlying provider when this observer gets notified of OnItemUpdated().
// This will only happen the first time the ContentId of the udpated OfflineItem
// matches the ContentId of the OfflineItem passed into this class constructor.
class TriggerSingleReentrantUpdateHelper
    : public ScopedMockOfflineContentProvider::ScopedMockObserver {
 public:
  TriggerSingleReentrantUpdateHelper(
      OfflineContentProvider* provider,
      MockOfflineContentProvider* wrapped_provider,
      const OfflineItem& new_item)
      : ScopedMockObserver(provider),
        wrapped_provider_(wrapped_provider),
        new_item_(new_item) {}
  ~TriggerSingleReentrantUpdateHelper() override {}

  void OnItemUpdated(const OfflineItem& item) override {
    if (wrapped_provider_) {
      if (item.id == new_item_.id)
        wrapped_provider_->NotifyOnItemUpdated(new_item_);
      wrapped_provider_ = nullptr;
    }
    ScopedMockObserver::OnItemUpdated(item);
  }

 private:
  MockOfflineContentProvider* wrapped_provider_;
  OfflineItem new_item_;
};

class ThrottledOfflineContentProviderTest : public testing::Test {
 public:
  ThrottledOfflineContentProviderTest()
      : task_runner_(new base::TestMockTimeTaskRunner),
        handle_(task_runner_),
        delay_(base::TimeDelta::FromSeconds(1)),
        provider_(delay_, &wrapped_provider_),
        weak_ptr_factory_(this) {}
  ~ThrottledOfflineContentProviderTest() override {}

  MOCK_METHOD1(OnGetAllItemsDone,
               void(const OfflineContentProvider::OfflineItemList&));
  MOCK_METHOD1(OnGetItemByIdDone, void(const base::Optional<OfflineItem>&));

 protected:
  base::TimeTicks GetTimeThatWillAllowAnUpdate() {
    return base::TimeTicks::Now() - delay_ -
           base::TimeDelta::FromMilliseconds(1);
  }

  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle handle_;

  base::TimeDelta delay_;
  MockOfflineContentProvider wrapped_provider_;
  ThrottledOfflineContentProvider provider_;
  base::WeakPtrFactory<ThrottledOfflineContentProviderTest> weak_ptr_factory_;
};

TEST_F(ThrottledOfflineContentProviderTest, TestBasicPassthrough) {
  ScopedMockOfflineContentProvider::ScopedMockObserver observer(&provider_);

  ContentId id("1", "A");
  OfflineItem item(id);

  OfflineContentProvider::OfflineItemList items;
  items.push_back(item);

  testing::InSequence sequence;
  EXPECT_CALL(wrapped_provider_, OpenItem(id));
  EXPECT_CALL(wrapped_provider_, RemoveItem(id));
  EXPECT_CALL(wrapped_provider_, CancelDownload(id));
  EXPECT_CALL(wrapped_provider_, PauseDownload(id));
  EXPECT_CALL(wrapped_provider_, ResumeDownload(id, true));
  EXPECT_CALL(wrapped_provider_, GetVisualsForItem(id, _));
  wrapped_provider_.SetItems(items);
  provider_.OpenItem(id);
  provider_.RemoveItem(id);
  provider_.CancelDownload(id);
  provider_.PauseDownload(id);
  provider_.ResumeDownload(id, true);
  provider_.GetVisualsForItem(id, OfflineContentProvider::VisualsCallback());

  EXPECT_CALL(*this, OnGetAllItemsDone(items)).Times(1);
  provider_.GetAllItems(
      base::BindOnce(&ThrottledOfflineContentProviderTest::OnGetAllItemsDone,
                     weak_ptr_factory_.GetWeakPtr()));

  EXPECT_CALL(*this, OnGetItemByIdDone(base::make_optional(item))).Times(1);
  provider_.GetItemById(
      id,
      base::BindOnce(&ThrottledOfflineContentProviderTest::OnGetItemByIdDone,
                     weak_ptr_factory_.GetWeakPtr()));
  task_runner_->RunUntilIdle();
}

TEST_F(ThrottledOfflineContentProviderTest, TestRemoveCancelsUpdate) {
  ScopedMockOfflineContentProvider::ScopedMockObserver observer(&provider_);

  ContentId id("1", "A");
  OfflineItem item(id);

  EXPECT_CALL(observer, OnItemUpdated(item)).Times(0);
  EXPECT_CALL(observer, OnItemRemoved(id)).Times(1);

  provider_.set_last_update_time(base::TimeTicks::Now());
  wrapped_provider_.NotifyOnItemUpdated(item);
  wrapped_provider_.NotifyOnItemRemoved(id);
  task_runner_->FastForwardUntilNoTasksRemain();
}

TEST_F(ThrottledOfflineContentProviderTest, TestOnItemUpdatedSquashed) {
  ScopedMockOfflineContentProvider::ScopedMockObserver observer(&provider_);

  ContentId id1("1", "A");
  ContentId id2("2", "B");

  OfflineItem item1(id1);
  OfflineItem item2(id2);

  OfflineItem updated_item1(id1);
  updated_item1.title = "updated1";
  OfflineItem updated_item2(id2);
  updated_item2.title = "updated2";

  EXPECT_CALL(observer, OnItemUpdated(updated_item1)).Times(1);
  EXPECT_CALL(observer, OnItemUpdated(updated_item2)).Times(1);

  provider_.set_last_update_time(base::TimeTicks::Now());
  wrapped_provider_.NotifyOnItemUpdated(item1);
  wrapped_provider_.NotifyOnItemUpdated(item2);
  wrapped_provider_.NotifyOnItemUpdated(updated_item2);
  wrapped_provider_.NotifyOnItemUpdated(updated_item1);

  task_runner_->FastForwardUntilNoTasksRemain();
}

TEST_F(ThrottledOfflineContentProviderTest, TestGetItemByIdOverridesUpdate) {
  ScopedMockOfflineContentProvider::ScopedMockObserver observer(&provider_);

  ContentId id1("1", "A");
  ContentId id2("2", "B");

  OfflineItem item1(id1);
  OfflineItem item2(id2);

  OfflineItem updated_item1(id1);
  updated_item1.title = "updated1";

  std::vector<OfflineItem> items = {item1, item2};
  wrapped_provider_.SetItems(items);

  EXPECT_CALL(observer, OnItemUpdated(updated_item1)).Times(1);
  EXPECT_CALL(observer, OnItemUpdated(item2)).Times(1);

  provider_.set_last_update_time(base::TimeTicks::Now());
  wrapped_provider_.NotifyOnItemUpdated(item1);
  wrapped_provider_.NotifyOnItemUpdated(item2);

  items = {updated_item1, item2};
  wrapped_provider_.SetItems(items);

  auto single_item_callback = [](const base::Optional<OfflineItem>& item) {};
  provider_.GetItemById(id1, base::BindOnce(single_item_callback));

  provider_.set_last_update_time(GetTimeThatWillAllowAnUpdate());
  wrapped_provider_.NotifyOnItemUpdated(item2);

  task_runner_->FastForwardUntilNoTasksRemain();
}

TEST_F(ThrottledOfflineContentProviderTest, TestGetAllItemsOverridesUpdate) {
  ScopedMockOfflineContentProvider::ScopedMockObserver observer(&provider_);

  ContentId id1("1", "A");
  ContentId id2("2", "B");

  OfflineItem item1(id1);
  OfflineItem item2(id2);

  OfflineItem updated_item1(id1);
  updated_item1.title = "updated1";

  OfflineContentProvider::OfflineItemList items;
  items.push_back(updated_item1);
  items.push_back(item2);

  EXPECT_CALL(observer, OnItemUpdated(updated_item1)).Times(1);
  EXPECT_CALL(observer, OnItemUpdated(item2)).Times(1);

  wrapped_provider_.SetItems(items);
  provider_.set_last_update_time(base::TimeTicks::Now());
  wrapped_provider_.NotifyOnItemUpdated(item1);
  wrapped_provider_.NotifyOnItemUpdated(item2);

  auto callback = [](const OfflineContentProvider::OfflineItemList& items) {};
  provider_.GetAllItems(base::BindOnce(callback));

  task_runner_->FastForwardUntilNoTasksRemain();
}

TEST_F(ThrottledOfflineContentProviderTest, TestThrottleWorksProperly) {
  ScopedMockOfflineContentProvider::ScopedMockObserver observer(&provider_);

  ContentId id1("1", "A");

  OfflineItem item1(id1);

  OfflineItem item2(id1);
  item2.title = "updated1";

  OfflineItem item3(id1);
  item3.title = "updated2";

  OfflineItem item4(id1);
  item4.title = "updated3";

  {
    EXPECT_CALL(observer, OnItemUpdated(item1)).Times(1);
    provider_.set_last_update_time(GetTimeThatWillAllowAnUpdate());
    wrapped_provider_.NotifyOnItemUpdated(item1);
  }

  {
    EXPECT_CALL(observer, OnItemUpdated(item3)).Times(1);
    provider_.set_last_update_time(base::TimeTicks::Now());
    wrapped_provider_.NotifyOnItemUpdated(item2);
    wrapped_provider_.NotifyOnItemUpdated(item3);
    task_runner_->FastForwardBy(delay_);
  }

  {
    EXPECT_CALL(observer, OnItemUpdated(item4)).Times(1);
    provider_.set_last_update_time(GetTimeThatWillAllowAnUpdate());
    wrapped_provider_.NotifyOnItemUpdated(item4);
    task_runner_->FastForwardUntilNoTasksRemain();
  }
}

TEST_F(ThrottledOfflineContentProviderTest, TestInitialRequestGoesThrough) {
  ScopedMockOfflineContentProvider::ScopedMockObserver observer(&provider_);

  ContentId id1("1", "A");

  OfflineItem item1(id1);

  OfflineItem item1_updated(id1);
  item1_updated.title = "updated1";

  {
    EXPECT_CALL(observer, OnItemUpdated(item1)).Times(1);
    provider_.set_last_update_time(GetTimeThatWillAllowAnUpdate());
    wrapped_provider_.NotifyOnItemUpdated(item1);
  }

  {
    EXPECT_CALL(observer, OnItemUpdated(_)).Times(0);
    provider_.set_last_update_time(base::TimeTicks::Now());
    wrapped_provider_.NotifyOnItemUpdated(item1_updated);
  }

  {
    EXPECT_CALL(observer, OnItemUpdated(item1_updated)).Times(1);
    task_runner_->FastForwardUntilNoTasksRemain();
  }
}

TEST_F(ThrottledOfflineContentProviderTest, TestReentrantUpdatesGetQueued) {
  ContentId id("1", "A");

  OfflineItem item(id);
  OfflineItem updated_item(id);
  updated_item.title = "updated";

  TriggerSingleReentrantUpdateHelper observer(&provider_, &wrapped_provider_,
                                              updated_item);
  {
    wrapped_provider_.NotifyOnItemUpdated(item);
    EXPECT_CALL(observer, OnItemUpdated(item)).Times(1);
    task_runner_->FastForwardBy(delay_);
  }

  {
    EXPECT_CALL(observer, OnItemUpdated(updated_item)).Times(1);
    task_runner_->FastForwardUntilNoTasksRemain();
  }
}

TEST_F(ThrottledOfflineContentProviderTest, TestPokingProviderFlushesQueue) {
  ScopedMockOfflineContentProvider::ScopedMockObserver observer(&provider_);

  ContentId id1("1", "A");
  OfflineItem item1(id1);

  OfflineItem item2(ContentId("2", "B"));
  OfflineItem item3(ContentId("3", "C"));
  OfflineItem item4(ContentId("4", "D"));
  OfflineItem item5(ContentId("5", "E"));
  OfflineItem item6(ContentId("6", "F"));

  auto updater = base::Bind(&MockOfflineContentProvider::NotifyOnItemUpdated,
                            base::Unretained(&wrapped_provider_));

  // Set up reentrancy calls back into the provider.
  EXPECT_CALL(wrapped_provider_, OpenItem(_))
      .WillRepeatedly(
          InvokeWithoutArgs(CallbackToFunctor(base::Bind(updater, item2))));
  EXPECT_CALL(wrapped_provider_, RemoveItem(_))
      .WillRepeatedly(
          InvokeWithoutArgs(CallbackToFunctor(base::Bind(updater, item3))));
  EXPECT_CALL(wrapped_provider_, CancelDownload(_))
      .WillRepeatedly(
          InvokeWithoutArgs(CallbackToFunctor(base::Bind(updater, item4))));
  EXPECT_CALL(wrapped_provider_, PauseDownload(_))
      .WillRepeatedly(
          InvokeWithoutArgs(CallbackToFunctor(base::Bind(updater, item5))));
  EXPECT_CALL(wrapped_provider_, ResumeDownload(_, _))
      .WillRepeatedly(
          InvokeWithoutArgs(CallbackToFunctor(base::Bind(updater, item6))));

  {
    EXPECT_CALL(observer, OnItemUpdated(item1)).Times(1);
    EXPECT_CALL(observer, OnItemUpdated(item2)).Times(1);
    provider_.set_last_update_time(base::TimeTicks::Now());
    wrapped_provider_.NotifyOnItemUpdated(item1);
    provider_.OpenItem(id1);
  }

  {
    EXPECT_CALL(observer, OnItemUpdated(item1)).Times(1);
    EXPECT_CALL(observer, OnItemUpdated(item3)).Times(1);
    provider_.set_last_update_time(base::TimeTicks::Now());
    wrapped_provider_.NotifyOnItemUpdated(item1);
    provider_.RemoveItem(id1);
  }

  {
    EXPECT_CALL(observer, OnItemUpdated(item1)).Times(1);
    EXPECT_CALL(observer, OnItemUpdated(item4)).Times(1);
    provider_.set_last_update_time(base::TimeTicks::Now());
    wrapped_provider_.NotifyOnItemUpdated(item1);
    provider_.CancelDownload(id1);
  }

  {
    EXPECT_CALL(observer, OnItemUpdated(item1)).Times(1);
    EXPECT_CALL(observer, OnItemUpdated(item5)).Times(1);
    provider_.set_last_update_time(base::TimeTicks::Now());
    wrapped_provider_.NotifyOnItemUpdated(item1);
    provider_.PauseDownload(id1);
  }

  {
    EXPECT_CALL(observer, OnItemUpdated(item1)).Times(1);
    EXPECT_CALL(observer, OnItemUpdated(item6)).Times(1);
    provider_.set_last_update_time(base::TimeTicks::Now());
    wrapped_provider_.NotifyOnItemUpdated(item1);
    provider_.ResumeDownload(id1, false);
  }
}

}  // namespace
}  // namespace offline_items_collection;
