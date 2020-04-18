// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_COMMON_THREAD_SCHEDULER_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_COMMON_THREAD_SCHEDULER_IMPL_H_

#include "third_party/blink/renderer/platform/platform_export.h"

#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"

namespace base {
class TickClock;

namespace sequence_manager {
class TimeDomain;
}
}  // namespace base

namespace blink {
namespace scheduler {

// Scheduler-internal interface for the common methods between
// MainThreadSchedulerImpl and NonMainThreadSchedulerImpl which should
// not be exposed outside the scheduler.
class PLATFORM_EXPORT ThreadSchedulerImpl : virtual public ThreadScheduler,
                                            virtual public WebThreadScheduler {
 public:
  virtual scoped_refptr<base::SingleThreadTaskRunner> ControlTaskRunner() = 0;

  virtual void RegisterTimeDomain(
      base::sequence_manager::TimeDomain* time_domain) = 0;
  virtual void UnregisterTimeDomain(
      base::sequence_manager::TimeDomain* time_domain) = 0;
  virtual base::sequence_manager::TimeDomain* GetActiveTimeDomain() = 0;

  virtual const base::TickClock* GetTickClock() = 0;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_COMMON_THREAD_SCHEDULER_IMPL_H_
