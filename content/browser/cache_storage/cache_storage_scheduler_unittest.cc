// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/cache_storage/cache_storage_scheduler.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/run_loop.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace cache_storage_scheduler_unittest {

class TestTask {
 public:
  TestTask(CacheStorageScheduler* scheduler)
      : scheduler_(scheduler), callback_count_(0) {}

  virtual void Run() { callback_count_++; }
  void Done() { scheduler_->CompleteOperationAndRunNext(); }

  int callback_count() const { return callback_count_; }

 protected:
  CacheStorageScheduler* scheduler_;
  int callback_count_;
};

class CacheStorageSchedulerTest : public testing::Test {
 protected:
  CacheStorageSchedulerTest()
      : browser_thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP),
        scheduler_(CacheStorageSchedulerClient::CLIENT_STORAGE),
        task1_(TestTask(&scheduler_)),
        task2_(TestTask(&scheduler_)) {}

  TestBrowserThreadBundle browser_thread_bundle_;
  CacheStorageScheduler scheduler_;
  TestTask task1_;
  TestTask task2_;
};

TEST_F(CacheStorageSchedulerTest, ScheduleOne) {
  scheduler_.ScheduleOperation(
      base::BindOnce(&TestTask::Run, base::Unretained(&task1_)));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, task1_.callback_count());
}

TEST_F(CacheStorageSchedulerTest, ScheduleTwo) {
  scheduler_.ScheduleOperation(
      base::BindOnce(&TestTask::Run, base::Unretained(&task1_)));
  scheduler_.ScheduleOperation(
      base::BindOnce(&TestTask::Run, base::Unretained(&task2_)));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, task1_.callback_count());
  EXPECT_EQ(0, task2_.callback_count());

  task1_.Done();
  EXPECT_TRUE(scheduler_.ScheduledOperations());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, task1_.callback_count());
  EXPECT_EQ(1, task2_.callback_count());
}

TEST_F(CacheStorageSchedulerTest, ScheduledOperations) {
  scheduler_.ScheduleOperation(
      base::BindOnce(&TestTask::Run, base::Unretained(&task1_)));
  EXPECT_TRUE(scheduler_.ScheduledOperations());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, task1_.callback_count());
  EXPECT_TRUE(scheduler_.ScheduledOperations());
  task1_.Done();
  EXPECT_FALSE(scheduler_.ScheduledOperations());
}

}  // namespace cache_storage_scheduler_unittest
}  // namespace content
