// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/task_test_base.h"

#include "base/test/mock_callback.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;

namespace offline_pages {

TaskTestBase::TaskTestBase()
    : task_runner_(new base::TestMockTimeTaskRunner),
      task_runner_handle_(task_runner_),
      test_task_runner_(task_runner_) {}

TaskTestBase::~TaskTestBase() = default;

void TaskTestBase::SetUp() {
  testing::Test::SetUp();
}

void TaskTestBase::TearDown() {
  task_runner_->FastForwardUntilNoTasksRemain();
  testing::Test::TearDown();
}

void TaskTestBase::RunUntilIdle() {
  task_runner_->RunUntilIdle();
}

void TaskTestBase::RunTask(std::unique_ptr<Task> task) {
  test_task_runner_.RunTask(std::move(task));
}

void TaskTestBase::RunTask(Task* task) {
  test_task_runner_.RunTask(task);
}

}  // namespace offline_pages
