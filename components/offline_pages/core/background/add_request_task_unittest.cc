// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/add_request_task.h"

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
const GURL kUrl2("http://otherexample.com");
const ClientId kClientId1("download", "1234");
const ClientId kClientId2("download", "5678");
}  // namespace

class AddRequestTaskTest : public testing::Test {
 public:
  AddRequestTaskTest();
  ~AddRequestTaskTest() override;

  void PumpLoop();
  void ClearResults();

  void InitializeStore(RequestQueueStore* store);

  void AddRequestDone(ItemActionStatus status);

  void GetRequestsCallback(
      bool success,
      std::vector<std::unique_ptr<SavePageRequest>> requests);

  bool callback_called() const { return callback_called_; }

  ItemActionStatus last_status() const { return status_; }

  const std::vector<std::unique_ptr<SavePageRequest>>& last_requests() const {
    return requests_;
  }

 private:
  void InitializeStoreDone(bool success);

  bool callback_called_;
  ItemActionStatus status_;
  std::vector<std::unique_ptr<SavePageRequest>> requests_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

AddRequestTaskTest::AddRequestTaskTest()
    : callback_called_(false),
      status_(ItemActionStatus::NOT_FOUND),
      task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {}

AddRequestTaskTest::~AddRequestTaskTest() {}

void AddRequestTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void AddRequestTaskTest::ClearResults() {
  callback_called_ = false;
  status_ = ItemActionStatus::NOT_FOUND;
  requests_.clear();
}

void AddRequestTaskTest::InitializeStore(RequestQueueStore* store) {
  store->Initialize(base::BindOnce(&AddRequestTaskTest::InitializeStoreDone,
                                   base::Unretained(this)));
  PumpLoop();
}

void AddRequestTaskTest::AddRequestDone(ItemActionStatus status) {
  status_ = status;
  callback_called_ = true;
}

void AddRequestTaskTest::GetRequestsCallback(
    bool success,
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  requests_ = std::move(requests);
}

void AddRequestTaskTest::InitializeStoreDone(bool success) {
  ASSERT_TRUE(success);
}

TEST_F(AddRequestTaskTest, AddSingleRequest) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);
  base::Time creation_time = base::Time::Now();
  SavePageRequest request_1(kRequestId1, kUrl1, kClientId1, creation_time,
                            true);
  AddRequestTask task(&store, request_1,
                      base::BindOnce(&AddRequestTaskTest::AddRequestDone,
                                     base::Unretained(this)));
  task.Run();
  PumpLoop();
  EXPECT_TRUE(callback_called());
  EXPECT_EQ(ItemActionStatus::SUCCESS, last_status());

  store.GetRequests(base::BindOnce(&AddRequestTaskTest::GetRequestsCallback,
                                   base::Unretained(this)));
  PumpLoop();
  ASSERT_EQ(1ul, last_requests().size());
  EXPECT_EQ(kRequestId1, last_requests().at(0)->request_id());
  EXPECT_EQ(kUrl1, last_requests().at(0)->url());
  EXPECT_EQ(kClientId1, last_requests().at(0)->client_id());
  EXPECT_EQ(creation_time, last_requests().at(0)->creation_time());
  EXPECT_TRUE(last_requests().at(0)->user_requested());
}

TEST_F(AddRequestTaskTest, AddMultipleRequests) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);
  base::Time creation_time_1 = base::Time::Now();
  SavePageRequest request_1(kRequestId1, kUrl1, kClientId1, creation_time_1,
                            true);
  AddRequestTask task(&store, request_1,
                      base::BindOnce(&AddRequestTaskTest::AddRequestDone,
                                     base::Unretained(this)));
  task.Run();
  PumpLoop();
  EXPECT_TRUE(callback_called());
  EXPECT_EQ(ItemActionStatus::SUCCESS, last_status());

  ClearResults();
  base::Time creation_time_2 = base::Time::Now();
  SavePageRequest request_2(kRequestId2, kUrl2, kClientId2, creation_time_2,
                            true);
  AddRequestTask task_2(&store, request_2,
                        base::BindOnce(&AddRequestTaskTest::AddRequestDone,
                                       base::Unretained(this)));
  task_2.Run();
  PumpLoop();
  EXPECT_TRUE(callback_called());
  EXPECT_EQ(ItemActionStatus::SUCCESS, last_status());

  store.GetRequests(base::BindOnce(&AddRequestTaskTest::GetRequestsCallback,
                                   base::Unretained(this)));
  PumpLoop();
  ASSERT_EQ(2ul, last_requests().size());
  int request_2_index =
      last_requests().at(0)->request_id() == kRequestId2 ? 0 : 1;
  EXPECT_EQ(kRequestId2, last_requests().at(request_2_index)->request_id());
  EXPECT_EQ(kUrl2, last_requests().at(request_2_index)->url());
  EXPECT_EQ(kClientId2, last_requests().at(request_2_index)->client_id());
  EXPECT_EQ(creation_time_2,
            last_requests().at(request_2_index)->creation_time());
  EXPECT_TRUE(last_requests().at(request_2_index)->user_requested());
}

TEST_F(AddRequestTaskTest, AddDuplicateRequest) {
  RequestQueueInMemoryStore store;
  InitializeStore(&store);
  base::Time creation_time_1 = base::Time::Now();
  SavePageRequest request_1(kRequestId1, kUrl1, kClientId1, creation_time_1,
                            true);
  AddRequestTask task(&store, request_1,
                      base::BindOnce(&AddRequestTaskTest::AddRequestDone,
                                     base::Unretained(this)));
  task.Run();
  PumpLoop();
  EXPECT_TRUE(callback_called());
  EXPECT_EQ(ItemActionStatus::SUCCESS, last_status());

  ClearResults();
  base::Time creation_time_2 = base::Time::Now();
  // This was has the same request ID.
  SavePageRequest request_2(kRequestId1, kUrl2, kClientId2, creation_time_2,
                            true);
  AddRequestTask task_2(&store, request_2,
                        base::BindOnce(&AddRequestTaskTest::AddRequestDone,
                                       base::Unretained(this)));
  task_2.Run();
  PumpLoop();
  EXPECT_TRUE(callback_called());
  EXPECT_EQ(ItemActionStatus::ALREADY_EXISTS, last_status());

  store.GetRequests(base::BindOnce(&AddRequestTaskTest::GetRequestsCallback,
                                   base::Unretained(this)));
  PumpLoop();
  ASSERT_EQ(1ul, last_requests().size());
}

}  // namespace offline_pages
