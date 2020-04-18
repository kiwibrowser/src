// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"

#include <memory>

#include "base/single_thread_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager_impl.h"
#include "third_party/blink/renderer/platform/scheduler/base/test/task_queue_manager_for_test.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/test/lazy_thread_controller_for_test.h"

namespace blink {
namespace scheduler {

std::unique_ptr<WebMainThreadScheduler> CreateWebMainThreadSchedulerForTests() {
  return std::make_unique<scheduler::MainThreadSchedulerImpl>(
      std::make_unique<base::sequence_manager::TaskQueueManagerForTest>(
          std::make_unique<LazyThreadControllerForTest>()),
      base::nullopt);
}

void RunIdleTasksForTesting(WebMainThreadScheduler* scheduler,
                            base::OnceClosure callback) {
  MainThreadSchedulerImpl* scheduler_impl =
      static_cast<MainThreadSchedulerImpl*>(scheduler);
  scheduler_impl->RunIdleTasksForTesting(std::move(callback));
}

scoped_refptr<base::SequencedTaskRunner> GetSequencedTaskRunnerForTesting() {
  return base::SequencedTaskRunnerHandle::Get();
}

scoped_refptr<base::SingleThreadTaskRunner>
GetSingleThreadTaskRunnerForTesting() {
  return base::ThreadTaskRunnerHandle::Get();
}

}  // namespace scheduler
}  // namespace blink
