// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_UTILITY_WEBTHREAD_IMPL_FOR_UTILITY_THREAD_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_UTILITY_WEBTHREAD_IMPL_FOR_UTILITY_THREAD_H_

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/scheduler/child/webthread_base.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {
namespace scheduler {

class PLATFORM_EXPORT WebThreadImplForUtilityThread
    : public scheduler::WebThreadBase {
 public:
  WebThreadImplForUtilityThread();
  ~WebThreadImplForUtilityThread() override;

  // WebThread implementation.
  ThreadScheduler* Scheduler() const override;
  PlatformThreadId ThreadId() const override;
  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner() const override;

  // WebThreadBase implementation.
  scheduler::SingleThreadIdleTaskRunner* GetIdleTaskRunner() const override;
  void Init() override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  PlatformThreadId thread_id_;

  DISALLOW_COPY_AND_ASSIGN(WebThreadImplForUtilityThread);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_UTILITY_WEBTHREAD_IMPL_FOR_UTILITY_THREAD_H_
