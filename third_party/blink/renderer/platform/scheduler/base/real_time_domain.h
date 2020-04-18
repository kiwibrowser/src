// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_REAL_TIME_DOMAIN_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_REAL_TIME_DOMAIN_H_

#include <set>

#include "base/macros.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/scheduler/base/time_domain.h"

namespace base {
namespace sequence_manager {

class PLATFORM_EXPORT RealTimeDomain : public TimeDomain {
 public:
  RealTimeDomain();
  ~RealTimeDomain() override;

  // TimeDomain implementation:
  LazyNow CreateLazyNow() const override;
  TimeTicks Now() const override;
  Optional<TimeDelta> DelayTillNextTask(LazyNow* lazy_now) override;
  const char* GetName() const override;

 protected:
  void OnRegisterWithTaskQueueManager(
      TaskQueueManagerImpl* task_queue_manager) override;
  void RequestWakeUpAt(TimeTicks now, TimeTicks run_time) override;
  void CancelWakeUpAt(TimeTicks run_time) override;
  void AsValueIntoInternal(trace_event::TracedValue* state) const override;

 private:
  TaskQueueManagerImpl* task_queue_manager_;  // NOT OWNED

  DISALLOW_COPY_AND_ASSIGN(RealTimeDomain);
};

}  // namespace sequence_manager
}  // namespace base

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_REAL_TIME_DOMAIN_H_
