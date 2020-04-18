// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/test_task.h"

#include "base/bind.h"

namespace offline_pages {

ConsumedResource::ConsumedResource() {}

ConsumedResource::~ConsumedResource() {}

void ConsumedResource::Step(const base::Closure& step_callback) {
  next_step_ = step_callback;
}

void ConsumedResource::CompleteStep() {
  base::Closure temp_ = next_step_;
  next_step_.Reset();
  temp_.Run();
}

TestTask::TestTask(ConsumedResource* resource)
    : resource_(resource),
      state_(TaskState::NOT_STARTED),
      leave_early_(false) {}

TestTask::TestTask(ConsumedResource* resource, bool leave_early)
    : resource_(resource),
      state_(TaskState::NOT_STARTED),
      leave_early_(leave_early) {}

TestTask::~TestTask() {}

// Run is Step 1 in our case.
void TestTask::Run() {
  state_ = TaskState::STEP_1;
  resource_->Step(base::Bind(&TestTask::Step2, base::Unretained(this)));
}

void TestTask::Step2() {
  if (leave_early_) {
    LastStep();
    return;
  }
  state_ = TaskState::STEP_2;
  resource_->Step(base::Bind(&TestTask::LastStep, base::Unretained(this)));
}

// This is step 3, but we conclude here.
void TestTask::LastStep() {
  state_ = TaskState::COMPLETED;
  TaskComplete();
}

}  // namespace offline_pages
