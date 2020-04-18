// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_TASK_TEST_BASE_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_TASK_TEST_BASE_H_

#include "testing/gtest/include/gtest/gtest.h"

#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/task.h"
#include "components/offline_pages/core/test_task_runner.h"

namespace offline_pages {

// Base class for testing prefetch requests with simulated responses.
class TaskTestBase : public testing::Test {
 public:
  TaskTestBase();
  ~TaskTestBase() override;

  void SetUp() override;
  void TearDown() override;

  // Runs task with expectation that it correctly completes.
  // Task is also cleaned up after completing.
  void RunTask(std::unique_ptr<Task> task);
  // Runs task with expectation that it correctly completes.
  // Task is not cleaned up after completing.
  void RunTask(Task* task);
  void RunUntilIdle();

  scoped_refptr<base::TestMockTimeTaskRunner> task_runner() {
    return task_runner_;
  }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  TestTaskRunner test_task_runner_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_TASK_TEST_BASE_H_
