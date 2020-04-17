// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_PRIORITIZED_TASK_RUNNER_H_
#define NET_BASE_PRIORITIZED_TASK_RUNNER_H_

#include <stdint.h>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/post_task_and_reply_with_result_internal.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/net_export.h"

namespace base {
class TaskRunner;
}  // namespace base

namespace net {

namespace internal {
template <typename ReturnType>
void ReturnAsParamAdapter(base::OnceCallback<ReturnType()> func,
                          ReturnType* result) {
  *result = std::move(func).Run();
}

// Adapts a T* result to a callblack that expects a T.
template <typename TaskReturnType, typename ReplyArgType>
void ReplyAdapter(base::OnceCallback<void(ReplyArgType)> callback,
                  TaskReturnType* result) {
  std::move(callback).Run(std::move(*result));
}
}  // namespace internal

// PrioritizedTaskRunner allows for prioritization of posted tasks within a
// single TaskRunner. Be careful, as it is possible to starve a task.
class NET_EXPORT_PRIVATE PrioritizedTaskRunner
    : public base::RefCountedThreadSafe<PrioritizedTaskRunner> {
 public:
  PrioritizedTaskRunner(scoped_refptr<base::TaskRunner> task_runner);

  // Similar to TaskRunner::PostTaskAndReply, except that the task runs at
  // |priority|. Priority 0 is the highest priority and will run before other
  // priority values. Multiple tasks with the same |priority| value are run in
  // arbitrary order.
  void PostTaskAndReply(const base::Location& from_here,
                        base::OnceClosure task,
                        base::OnceClosure reply,
                        uint32_t priority);

  // Similar to base::PostTaskAndReplyWithResult, except that the task runs at
  // |priority|. See PostTaskAndReply for a description of |priority|.
  template <typename TaskReturnType, typename ReplyArgType>
  void PostTaskAndReplyWithResult(const base::Location& from_here,
                                  base::OnceCallback<TaskReturnType()> task,
                                  base::OnceCallback<void(ReplyArgType)> reply,
                                  uint32_t priority) {
    TaskReturnType* result = new TaskReturnType();
    return PostTaskAndReply(
        from_here,
        BindOnce(&internal::ReturnAsParamAdapter<TaskReturnType>,
                 std::move(task), result),
        BindOnce(&internal::ReplyAdapter<TaskReturnType, ReplyArgType>,
                 std::move(reply), base::Owned(result)),
        priority);
  }

  base::TaskRunner* task_runner() { return task_runner_.get(); }

 private:
  friend class base::RefCountedThreadSafe<PrioritizedTaskRunner>;

  struct Job {
    Job(const base::Location& from_here,
        base::OnceClosure task,
        base::OnceClosure reply,
        uint32_t priority);
    ~Job();

    Job(Job&& other);
    Job& operator=(Job&& other);

    base::Location from_here;
    base::OnceClosure task;
    base::OnceClosure reply;
    uint32_t priority;
    scoped_refptr<base::TaskRunner> reply_task_runner =
        base::ThreadTaskRunnerHandle::Get();

   private:
    DISALLOW_COPY_AND_ASSIGN(Job);
  };

  struct JobComparer {
    bool operator()(const Job& left, const Job& right) {
      return left.priority > right.priority;
    }
  };

  // Pops the next task from the heap.
  Job PopJob();

  // Runs the highest priority job available.
  void RunPostTaskAndReply();

  ~PrioritizedTaskRunner();

  // TODO(jkarlin): Replace the heap with a std::priority_queue once it
  // supports move-only types.
  // Accessed on both task_runner_ and Job::reply_task_runner.
  std::vector<Job> job_heap_;
  base::Lock job_heap_lock_;

  // Accessed on Job::reply_task_runner.
  scoped_refptr<base::TaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(PrioritizedTaskRunner);
};

}  // namespace net

#endif  // NET_BASE_PRIORITIZED_TASK_RUNNER_H_
