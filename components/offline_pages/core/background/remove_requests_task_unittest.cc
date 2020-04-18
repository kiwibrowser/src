// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/remove_requests_task.h"

#include <memory>

#include "base/bind.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/background/request_queue_in_memory_store.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {
namespace {
const int64_t kRequestId1 = 42;
const int64_t kRequestId2 = 43;
const int64_t kRequestId3 = 44;
const GURL kUrl1("http://example.com");
const GURL kUrl2("http://another-example.com");
const ClientId kClientId1("bookmark", "1234");
const ClientId kClientId2("async", "5678");
}  // namespace

class RemoveRequestsTaskTest : public testing::Test {
 public:
  RemoveRequestsTaskTest();
  ~RemoveRequestsTaskTest() override;

  void PumpLoop();

  void InitializeStore(RequestQueueStore* store);
  void AddRequestsToStore(RequestQueueStore* store);
  void RemoveRequestsCallback(std::unique_ptr<UpdateRequestsResult> result);

  UpdateRequestsResult* last_result() const { return result_.get(); }

 private:
  void InitializeStoreDone(bool succesS);
  void AddRequestDone(ItemActionStatus status);

  std::unique_ptr<UpdateRequestsResult> result_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

RemoveRequestsTaskTest::RemoveRequestsTaskTest()
    : task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {}

RemoveRequestsTaskTest::~RemoveRequestsTaskTest() {}

void RemoveRequestsTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void RemoveRequestsTaskTest::InitializeStore(RequestQueueStore* store) {
  store->Initialize(base::BindOnce(&RemoveRequestsTaskTest::InitializeStoreDone,
                                   base::Unretained(this)));
  PumpLoop();
}

void RemoveRequestsTaskTest::AddRequestsToStore(RequestQueueStore* store) {
  base::Time creation_time = base::Time::Now();
  SavePageRequest request_1(kRequestId1, kUrl1, kClientId1, creation_time,
                            true);
  store->AddRequest(request_1,
                    base::BindOnce(&RemoveRequestsTaskTest::AddRequestDone,
                                   base::Unretained(this)));
  SavePageRequest request_2(kRequestId2, kUrl2, kClientId2, creation_time,
                            true);
  store->AddRequest(request_2,
                    base::BindOnce(&RemoveRequestsTaskTest::AddRequestDone,
                                   base::Unretained(this)));
  PumpLoop();
}

void RemoveRequestsTaskTest::RemoveRequestsCallback(
    std::unique_ptr<UpdateRequestsResult> result) {
  result_ = std::move(result);
}

void RemoveRequestsTaskTest::InitializeStoreDone(bool success) {
  ASSERT_TRUE(success);
}

void RemoveRequestsTaskTest::AddRequestDone(ItemActionStatus status) {
  ASSERT_EQ(ItemActionStatus::SUCCESS, status);
}

TEST_F(RemoveRequestsTaskTest, RemoveWhenStoreEmpty) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);

  std::vector<int64_t> request_ids{kRequestId1};
  RemoveRequestsTask task(
      &store, request_ids,
      base::BindOnce(&RemoveRequestsTaskTest::RemoveRequestsCallback,
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

TEST_F(RemoveRequestsTaskTest, RemoveSingleItem) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);
  AddRequestsToStore(&store);

  std::vector<int64_t> request_ids{kRequestId1};
  RemoveRequestsTask task(
      &store, request_ids,
      base::BindOnce(&RemoveRequestsTaskTest::RemoveRequestsCallback,
                     base::Unretained(this)));
  task.Run();
  PumpLoop();
  ASSERT_TRUE(last_result());
  EXPECT_EQ(1UL, last_result()->item_statuses.size());
  EXPECT_EQ(kRequestId1, last_result()->item_statuses.at(0).first);
  EXPECT_EQ(ItemActionStatus::SUCCESS,
            last_result()->item_statuses.at(0).second);
  EXPECT_EQ(1UL, last_result()->updated_items.size());
  EXPECT_EQ(kRequestId1, last_result()->updated_items.at(0).request_id());
}

TEST_F(RemoveRequestsTaskTest, RemoveMultipleItems) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);
  AddRequestsToStore(&store);

  std::vector<int64_t> request_ids{kRequestId1, kRequestId2};
  RemoveRequestsTask task(
      &store, request_ids,
      base::BindOnce(&RemoveRequestsTaskTest::RemoveRequestsCallback,
                     base::Unretained(this)));
  task.Run();
  PumpLoop();
  ASSERT_TRUE(last_result());
  EXPECT_EQ(2UL, last_result()->item_statuses.size());
  EXPECT_EQ(kRequestId1, last_result()->item_statuses.at(0).first);
  EXPECT_EQ(ItemActionStatus::SUCCESS,
            last_result()->item_statuses.at(0).second);
  EXPECT_EQ(kRequestId2, last_result()->item_statuses.at(1).first);
  EXPECT_EQ(ItemActionStatus::SUCCESS,
            last_result()->item_statuses.at(1).second);
  EXPECT_EQ(2UL, last_result()->updated_items.size());
  EXPECT_EQ(kRequestId1, last_result()->updated_items.at(0).request_id());
  EXPECT_EQ(kRequestId2, last_result()->updated_items.at(1).request_id());
}

TEST_F(RemoveRequestsTaskTest, DeleteWithEmptyIdList) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);

  std::vector<int64_t> request_ids;
  RemoveRequestsTask task(
      &store, request_ids,
      base::BindOnce(&RemoveRequestsTaskTest::RemoveRequestsCallback,
                     base::Unretained(this)));
  task.Run();
  PumpLoop();
  ASSERT_TRUE(last_result());
  EXPECT_EQ(0UL, last_result()->item_statuses.size());
  EXPECT_EQ(0UL, last_result()->updated_items.size());
}

TEST_F(RemoveRequestsTaskTest, RemoveMissingItem) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);
  AddRequestsToStore(&store);

  std::vector<int64_t> request_ids{kRequestId1, kRequestId3};
  RemoveRequestsTask task(
      &store, request_ids,
      base::BindOnce(&RemoveRequestsTaskTest::RemoveRequestsCallback,
                     base::Unretained(this)));
  task.Run();
  PumpLoop();
  ASSERT_TRUE(last_result());
  EXPECT_EQ(2UL, last_result()->item_statuses.size());
  EXPECT_EQ(kRequestId1, last_result()->item_statuses.at(0).first);
  EXPECT_EQ(ItemActionStatus::SUCCESS,
            last_result()->item_statuses.at(0).second);
  EXPECT_EQ(kRequestId3, last_result()->item_statuses.at(1).first);
  EXPECT_EQ(ItemActionStatus::NOT_FOUND,
            last_result()->item_statuses.at(1).second);
  EXPECT_EQ(1UL, last_result()->updated_items.size());
  EXPECT_EQ(kRequestId1, last_result()->updated_items.at(0).request_id());
}

}  // namespace offline_pages
