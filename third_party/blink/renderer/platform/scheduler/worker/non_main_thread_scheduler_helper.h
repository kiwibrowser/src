// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_NON_MAIN_THREAD_SCHEDULER_HELPER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_NON_MAIN_THREAD_SCHEDULER_HELPER_H_

#include "third_party/blink/renderer/platform/scheduler/common/scheduler_helper.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_task_queue.h"

namespace blink {
namespace scheduler {

class NonMainThreadScheduler;

class PLATFORM_EXPORT NonMainThreadSchedulerHelper : public SchedulerHelper {
 public:
  NonMainThreadSchedulerHelper(
      std::unique_ptr<base::sequence_manager::TaskQueueManager> manager,
      NonMainThreadScheduler* non_main_thread_scheduler,
      TaskType default_task_type);
  ~NonMainThreadSchedulerHelper() override;

  scoped_refptr<WorkerTaskQueue> NewTaskQueue(
      const base::sequence_manager::TaskQueue::Spec& spec);

  scoped_refptr<WorkerTaskQueue> DefaultWorkerTaskQueue();
  scoped_refptr<WorkerTaskQueue> ControlWorkerTaskQueue();

 protected:
  scoped_refptr<base::sequence_manager::TaskQueue> DefaultTaskQueue() override;
  scoped_refptr<base::sequence_manager::TaskQueue> ControlTaskQueue() override;

 private:
  NonMainThreadScheduler* non_main_thread_scheduler_;  // NOT OWNED
  const scoped_refptr<WorkerTaskQueue> default_task_queue_;
  const scoped_refptr<WorkerTaskQueue> control_task_queue_;

  DISALLOW_COPY_AND_ASSIGN(NonMainThreadSchedulerHelper);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_NON_MAIN_THREAD_SCHEDULER_HELPER_H_
