// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_WEB_THREAD_SCHEDULER_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_WEB_THREAD_SCHEDULER_H_

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/scheduler/single_thread_idle_task_runner.h"
#include "third_party/blink/public/platform/web_common.h"

namespace blink {
namespace scheduler {

class BLINK_PLATFORM_EXPORT WebThreadScheduler {
 public:
  virtual ~WebThreadScheduler();

  // Returns the default task runner.
  virtual scoped_refptr<base::SingleThreadTaskRunner> DefaultTaskRunner() = 0;

  // Returns the idle task runner. Tasks posted to this runner may be reordered
  // relative to other task types and may be starved for an arbitrarily long
  // time if no idle time is available.
  virtual scoped_refptr<SingleThreadIdleTaskRunner> IdleTaskRunner() = 0;

  virtual scoped_refptr<base::SingleThreadTaskRunner> IPCTaskRunner() = 0;

  // Shuts down the scheduler by dropping any remaining pending work in the work
  // queues. After this call any work posted to the task runners will be
  // silently dropped.
  virtual void Shutdown() = 0;

 protected:
  WebThreadScheduler() = default;
  DISALLOW_COPY_AND_ASSIGN(WebThreadScheduler);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_SCHEDULER_WEB_THREAD_SCHEDULER_H_
