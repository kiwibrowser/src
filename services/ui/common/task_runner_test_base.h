// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_COMMON_TASK_RUNNER_TEST_BASE_H_
#define SERVICES_UI_COMMON_TASK_RUNNER_TEST_BASE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/test/test_mock_time_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

// Test base with a TaskRunner. The test implementation should not create its
// own MessageLoop, as this creates a MessageLoop and sets a TaskRunner. Useful
// for testing Mojo interfaces, or anything else where the test needs to wait
// for asynchronous calls to happen before checking side effects.
class TaskRunnerTestBase : public testing::Test {
 public:
  TaskRunnerTestBase();
  ~TaskRunnerTestBase() override;

  base::TestMockTimeTaskRunner* task_runner() { return task_runner_.get(); }

  // Run all tasks that are scheduled to start immediately.
  void RunUntilIdle();

  // Run all tasks that are scheduled to start within |delta| and forward clock
  // by |delta|.
  void RunTasksForNext(base::TimeDelta delta);

  // Run all scheduled tasks and forward clock to last task start time.
  void RunAllTasks();

  // testing::Test:
  void SetUp() override;

 private:
  base::MessageLoop message_loop_;

  // Added as the task runner for message loop.
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(TaskRunnerTestBase);
};

}  // namespace ui

#endif  // SERVICES_UI_COMMON_TASK_RUNNER_TEST_BASE_H_
