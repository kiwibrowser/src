/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_THREAD_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_THREAD_H_

#include "base/callback_forward.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_thread_type.h"

#include <stdint.h>

namespace base {
namespace sequence_manager {
class TaskTimeObserver;
}
}  // namespace base

namespace blink {

class FrameScheduler;
class ThreadScheduler;

// Always an integer value.
typedef uintptr_t PlatformThreadId;

struct BLINK_PLATFORM_EXPORT WebThreadCreationParams {
  explicit WebThreadCreationParams(WebThreadType);

  WebThreadCreationParams& SetThreadNameForTest(const char* name);

  // Sets a scheduler for the frame which was responsible for the creation
  // of this thread.
  WebThreadCreationParams& SetFrameScheduler(FrameScheduler*);

  WebThreadType thread_type;
  const char* name;
  FrameScheduler* frame_scheduler;  // NOT OWNED
  base::Thread::Options thread_options;
};

// Provides an interface to an embedder-defined thread implementation.
//
// Deleting the thread blocks until all pending, non-delayed tasks have been
// run.
class BLINK_PLATFORM_EXPORT WebThread {
 public:
  // An IdleTask is passed a deadline in CLOCK_MONOTONIC seconds and is
  // expected to complete before this deadline.
  using IdleTask = base::OnceCallback<void(double deadline_seconds)>;

  class BLINK_PLATFORM_EXPORT TaskObserver {
   public:
    virtual ~TaskObserver() = default;
    virtual void WillProcessTask() = 0;
    virtual void DidProcessTask() = 0;
  };

  // DEPRECATED: Returns a task runner bound to the underlying scheduler's
  // default task queue.
  //
  // Default scheduler task queue does not give scheduler enough freedom to
  // manage task priorities and should not be used.
  // Use ExecutionContext::GetTaskRunner instead (crbug.com/624696).
  virtual scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner() const {
    return nullptr;
  }

  virtual bool IsCurrentThread() const = 0;
  virtual PlatformThreadId ThreadId() const { return 0; }

  // TaskObserver is an object that receives task notifications from the
  // MessageLoop
  // NOTE: TaskObserver implementation should be extremely fast!
  // This API is performance sensitive. Use only if you have a compelling
  // reason.
  virtual void AddTaskObserver(TaskObserver*) {}
  virtual void RemoveTaskObserver(TaskObserver*) {}

  // TaskTimeObserver is an object that receives notifications for
  // CPU time spent in each top-level MessageLoop task.
  // NOTE: TaskTimeObserver implementation should be extremely fast!
  // This API is performance sensitive. Use only if you have a compelling
  // reason.
  virtual void AddTaskTimeObserver(base::sequence_manager::TaskTimeObserver*) {}
  virtual void RemoveTaskTimeObserver(
      base::sequence_manager::TaskTimeObserver*) {}

  // Returns the scheduler associated with the thread.
  virtual ThreadScheduler* Scheduler() const = 0;

  virtual ~WebThread() = default;
};

}  // namespace blink

#endif
