// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_THREAD_LIFECYCLE_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_THREAD_LIFECYCLE_CONTEXT_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/lifecycle_notifier.h"

namespace blink {

class WorkerThreadLifecycleObserver;

// Used for notifying observers on the creating thread of worker thread
// termination. The lifetime of this class is equal to that of WorkerThread.
// Created and destructed on the thread that constructed the worker.
class CORE_EXPORT WorkerThreadLifecycleContext final
    : public GarbageCollectedFinalized<WorkerThreadLifecycleContext>,
      public LifecycleNotifier<WorkerThreadLifecycleContext,
                               WorkerThreadLifecycleObserver> {
  USING_GARBAGE_COLLECTED_MIXIN(WorkerThreadLifecycleContext);

 public:
  WorkerThreadLifecycleContext();
  ~WorkerThreadLifecycleContext() override;
  void NotifyContextDestroyed() override;

 private:
  friend class WorkerThreadLifecycleObserver;
  bool was_context_destroyed_ = false;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(WorkerThreadLifecycleContext);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_THREAD_LIFECYCLE_CONTEXT_H_
