// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/mark_attempt_started_task.h"

#include <memory>

#include "base/bind.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/background/request_queue_in_memory_store.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {
namespace {
const int64_t kRequestId1 = 42;
const int64_t kRequestId2 = 44;
const GURL kUrl1("http://example.com");
const ClientId kClientId1("download", "1234");
}  // namespace

class MarkAttemptStartedTaskTest : public testing::Test {
 public:
  MarkAttemptStartedTaskTest();
  ~MarkAttemptStartedTaskTest() override;

  void PumpLoop();

  void InitializeStore(RequestQueueStore* store);
  void AddItemToStore(RequestQueueStore* store);
  void ChangeRequestsStateCallback(
      std::unique_ptr<UpdateRequestsResult> result);

  UpdateRequestsResult* last_result() const { return result_.get(); }

 private:
  void InitializeStoreDone(bool success);
  void AddRequestDone(ItemActionStatus status);

  std::unique_ptr<UpdateRequestsResult> result_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

MarkAttemptStartedTaskTest::MarkAttemptStartedTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {}

MarkAttemptStartedTaskTest::~MarkAttemptStartedTaskTest() {}

void MarkAttemptStartedTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void MarkAttemptStartedTaskTest::InitializeStore(RequestQueueStore* store) {
  store->Initialize(
      base::BindOnce(&MarkAttemptStartedTaskTest::InitializeStoreDone,
                     base::Unretained(this)));
  PumpLoop();
}

void MarkAttemptStartedTaskTest::AddItemToStore(RequestQueueStore* store) {
  base::Time creation_time = base::Time::Now();
  SavePageRequest request_1(kRequestId1, kUrl1, kClientId1, creation_time,
                            true);
  store->AddRequest(request_1,
                    base::BindOnce(&MarkAttemptStartedTaskTest::AddRequestDone,
                                   base::Unretained(this)));
  PumpLoop();
}

void MarkAttemptStartedTaskTest::ChangeRequestsStateCallback(
    std::unique_ptr<UpdateRequestsResult> result) {
  result_ = std::move(result);
}

void MarkAttemptStartedTaskTest::InitializeStoreDone(bool success) {
  ASSERT_TRUE(success);
}

void MarkAttemptStartedTaskTest::AddRequestDone(ItemActionStatus status) {
  ASSERT_EQ(ItemActionStatus::SUCCESS, status);
}

TEST_F(MarkAttemptStartedTaskTest, MarkAttemptStartedWhenStoreEmpty) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);

  MarkAttemptStartedTask task(
      &store, kRequestId1,
      base::BindOnce(&MarkAttemptStartedTaskTest::ChangeRequestsStateCallback,
                     base::Unretained(this)));
  task.Run();
  PumpLoop();
  ASSERT_TRUE(last_result());
  EXPECT_EQ(1UL, last_result()->item_statuses.size());
  EXPECT_EQ(kRequestId1, last_result()->item_statuses.at(0).first);
  EXPECT_EQ(ItemActionStatus::NOT_FOUND,
            last_result()->item_statuses.at(0).second);
  EXPECT_EQ(0UL, last_result()->updated_items.size());
}

TEST_F(MarkAttemptStartedTaskTest, MarkAttemptStartedWhenExists) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);
  AddItemToStore(&store);

  MarkAttemptStartedTask task(
      &store, kRequestId1,
      base::BindOnce(&MarkAttemptStartedTaskTest::ChangeRequestsStateCallback,
                     base::Unretained(this)));

  // Current time for verification.
  base::Time before_time = base::Time::Now();
  task.Run();
  PumpLoop();
  ASSERT_TRUE(last_result());
  EXPECT_EQ(1UL, last_result()->item_statuses.size());
  EXPECT_EQ(kRequestId1, last_result()->item_statuses.at(0).first);
  EXPECT_EQ(ItemActionStatus::SUCCESS,
            last_result()->item_statuses.at(0).second);
  EXPECT_EQ(1UL, last_result()->updated_items.size());
  EXPECT_LE(before_time,
            last_result()->updated_items.at(0).last_attempt_time());
  EXPECT_GE(base::Time::Now(),
            last_result()->updated_items.at(0).last_attempt_time());
  EXPECT_EQ(1, last_result()->updated_items.at(0).started_attempt_count());
  EXPECT_EQ(SavePageRequest::RequestState::OFFLINING,
            last_result()->updated_items.at(0).request_state());
}

TEST_F(MarkAttemptStartedTaskTest, MarkAttemptStartedWhenItemMissing) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);
  AddItemToStore(&store);

  MarkAttemptStartedTask task(
      &store, kRequestId2,
      base::BindOnce(&MarkAttemptStartedTaskTest::ChangeRequestsStateCallback,
                     base::Unretained(this)));
  task.Run();
  PumpLoop();
  ASSERT_TRUE(last_result());
  EXPECT_EQ(1UL, last_result()->item_statuses.size());
  EXPECT_EQ(kRequestId2, last_result()->item_statuses.at(0).first);
  EXPECT_EQ(ItemActionStatus::NOT_FOUND,
            last_result()->item_statuses.at(0).second);
  EXPECT_EQ(0UL, last_result()->updated_items.size());
}

}  // namespace offline_pages
