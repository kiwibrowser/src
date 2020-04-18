// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_VIRTUAL_TIME_DOMAIN_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_VIRTUAL_TIME_DOMAIN_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "third_party/blink/renderer/platform/scheduler/base/time_domain.h"

namespace base {
namespace sequence_manager {

class PLATFORM_EXPORT VirtualTimeDomain : public TimeDomain {
 public:
  explicit VirtualTimeDomain(TimeTicks initial_time_ticks);
  ~VirtualTimeDomain() override;

  // TimeDomain implementation:
  LazyNow CreateLazyNow() const override;
  TimeTicks Now() const override;
  Optional<TimeDelta> DelayTillNextTask(LazyNow* lazy_now) override;
  const char* GetName() const override;

  // Advances this time domain to |now|. NOTE |now| is supposed to be
  // monotonically increasing.  NOTE it's the responsibility of the caller to
  // call TaskQueueManager::MaybeScheduleImmediateWork if needed.
  void AdvanceNowTo(TimeTicks now);

 protected:
  void OnRegisterWithTaskQueueManager(
      TaskQueueManagerImpl* task_queue_manager) override;
  void RequestWakeUpAt(TimeTicks now, TimeTicks run_time) override;
  void CancelWakeUpAt(TimeTicks run_time) override;
  void AsValueIntoInternal(trace_event::TracedValue* state) const override;

  void RequestDoWork();

 private:
  mutable Lock lock_;  // Protects |now_ticks_|
  TimeTicks now_ticks_;

  TaskQueueManagerImpl* task_queue_manager_;  // NOT OWNED
  Closure do_work_closure_;

  DISALLOW_COPY_AND_ASSIGN(VirtualTimeDomain);
};

}  // namespace sequence_manager
}  // namespace base

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_VIRTUAL_TIME_DOMAIN_H_
