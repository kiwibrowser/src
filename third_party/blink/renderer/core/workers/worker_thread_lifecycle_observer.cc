// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/worker_thread_lifecycle_observer.h"

#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/core/workers/worker_thread_lifecycle_context.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/wtf.h"

namespace blink {

WorkerThreadLifecycleObserver::WorkerThreadLifecycleObserver(
    WorkerThreadLifecycleContext* worker_thread_lifecycle_context)
    : LifecycleObserver(worker_thread_lifecycle_context),
      was_context_destroyed_before_observer_creation_(
          worker_thread_lifecycle_context->was_context_destroyed_) {
}

WorkerThreadLifecycleObserver::~WorkerThreadLifecycleObserver() = default;

}  // namespace blink
