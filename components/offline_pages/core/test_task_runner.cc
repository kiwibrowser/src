// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/test_task_runner.h"

#include "base/bind.h"
#include "components/offline_pages/core/task.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

TestTaskRunner::TestTaskRunner(
    scoped_refptr<base::TestMockTimeTaskRunner> task_runner)
    : task_runner_(task_runner) {}

TestTaskRunner::~TestTaskRunner() {}

void TestTaskRunner::RunTask(std::unique_ptr<Task> task) {
  RunTask(task.get());
}

void TestTaskRunner::RunTask(Task* task) {
  DCHECK(task);
  Task* completed_task = nullptr;
  task->SetTaskCompletionCallbackForTesting(
      task_runner_.get(),
      base::Bind([](Task** completed_task_ptr,
                    Task* task) { *completed_task_ptr = task; },
                 &completed_task));
  task->Run();
  task_runner_->RunUntilIdle();
  EXPECT_EQ(task, completed_task);
}

}  // namespace offline_pages
