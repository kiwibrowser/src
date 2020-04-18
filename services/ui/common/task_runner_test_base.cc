// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/common/task_runner_test_base.h"

namespace ui {

TaskRunnerTestBase::TaskRunnerTestBase() {}

TaskRunnerTestBase::~TaskRunnerTestBase() {}

void TaskRunnerTestBase::RunUntilIdle() {
  task_runner_->RunUntilIdle();
}

void TaskRunnerTestBase::RunTasksForNext(base::TimeDelta delta) {
  task_runner_->FastForwardBy(delta);
}

void TaskRunnerTestBase::RunAllTasks() {
  task_runner_->FastForwardUntilNoTasksRemain();
}

void TaskRunnerTestBase::SetUp() {
  task_runner_ = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
      base::Time::Now(), base::TimeTicks::Now());
  message_loop_.SetTaskRunner(task_runner_);
}

}  // namespace ui
