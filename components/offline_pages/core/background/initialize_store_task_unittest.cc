// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/initialize_store_task.h"

#include <memory>

#include "base/bind.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/background/request_queue_in_memory_store.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

using TestScenario = RequestQueueInMemoryStore::TestScenario;

class InitializeStoreTaskTest : public testing::Test {
 public:
  InitializeStoreTaskTest();
  ~InitializeStoreTaskTest() override;

  void PumpLoop();
  void RunNextStep();

  void InitializeCallback(bool success);

  bool callback_called() const { return callback_called_; }

  bool last_call_successful() const { return success_; }

 private:
  bool callback_called_;
  bool success_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
};

InitializeStoreTaskTest::InitializeStoreTaskTest()
    : callback_called_(false),
      success_(false),
      task_runner_(new base::TestSimpleTaskRunner),
      task_runner_handle_(task_runner_) {}

InitializeStoreTaskTest::~InitializeStoreTaskTest() {}

void InitializeStoreTaskTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void InitializeStoreTaskTest::RunNextStep() {
  // Only runs tasks that are already scheduled and available. If running these
  // tasks post new tasks to the runner, they will not be triggered. This allows
  // for running InitializeStoreTask one step at a time.
  task_runner_->RunPendingTasks();
}

void InitializeStoreTaskTest::InitializeCallback(bool success) {
  callback_called_ = true;
  success_ = success;
}

TEST_F(InitializeStoreTaskTest, SuccessfulInitialization) {
  RequestQueueInMemoryStore store;
  InitializeStoreTask task(
      &store, base::BindOnce(&InitializeStoreTaskTest::InitializeCallback,
                             base::Unretained(this)));
  task.Run();
  PumpLoop();
  EXPECT_TRUE(callback_called());
  EXPECT_TRUE(last_call_successful());
  EXPECT_EQ(StoreState::LOADED, store.state());
}

TEST_F(InitializeStoreTaskTest, SuccessfulReset) {
  RequestQueueInMemoryStore store(TestScenario::LOAD_FAILED_RESET_SUCCESS);
  InitializeStoreTask task(
      &store, base::BindOnce(&InitializeStoreTaskTest::InitializeCallback,
                             base::Unretained(this)));
  task.Run();
  EXPECT_FALSE(callback_called());
  EXPECT_EQ(StoreState::FAILED_LOADING, store.state());

  RunNextStep();
  EXPECT_FALSE(callback_called());
  EXPECT_EQ(StoreState::NOT_LOADED, store.state());

  PumpLoop();
  EXPECT_TRUE(callback_called());
  EXPECT_TRUE(last_call_successful());
  EXPECT_EQ(StoreState::LOADED, store.state());
}

TEST_F(InitializeStoreTaskTest, FailedReset) {
  RequestQueueInMemoryStore store(TestScenario::LOAD_FAILED_RESET_FAILED);
  InitializeStoreTask task(
      &store, base::BindOnce(&InitializeStoreTaskTest::InitializeCallback,
                             base::Unretained(this)));
  task.Run();
  PumpLoop();
  EXPECT_TRUE(callback_called());
  EXPECT_FALSE(last_call_successful());
  EXPECT_EQ(StoreState::FAILED_RESET, store.state());
}

}  // namespace offline_pages
