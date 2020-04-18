// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_TASK_QUEUE_WITH_TASK_TYPE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_TASK_QUEUE_WITH_TASK_TYPE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace base {
namespace sequence_manager {
class TaskQueue;
}
}  // namespace base

namespace blink {
namespace scheduler {

class PLATFORM_EXPORT TaskQueueWithTaskType
    : public base::SingleThreadTaskRunner {
 public:
  static scoped_refptr<TaskQueueWithTaskType> Create(
      scoped_refptr<base::sequence_manager::TaskQueue> task_queue,
      TaskType task_type);

  // base::SingleThreadTaskRunner implementation:
  bool RunsTasksInCurrentSequence() const override;

 protected:
  bool PostDelayedTask(const base::Location&,
                       base::OnceClosure,
                       base::TimeDelta) override;
  bool PostNonNestableDelayedTask(const base::Location&,
                                  base::OnceClosure,
                                  base::TimeDelta) override;

 private:
  TaskQueueWithTaskType(
      scoped_refptr<base::sequence_manager::TaskQueue> task_queue,
      TaskType task_type);
  ~TaskQueueWithTaskType() override;

  scoped_refptr<base::sequence_manager::TaskQueue> task_queue_;
  TaskType task_type_;

  DISALLOW_COPY_AND_ASSIGN(TaskQueueWithTaskType);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_TASK_RUNNER_IMPL_H_
