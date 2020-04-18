// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/worker_thread_lifecycle_context.h"

#include "third_party/blink/renderer/core/workers/worker_thread_lifecycle_observer.h"

namespace blink {

WorkerThreadLifecycleContext::WorkerThreadLifecycleContext() {
  DETACH_FROM_THREAD(thread_checker_);
}

WorkerThreadLifecycleContext::~WorkerThreadLifecycleContext() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void WorkerThreadLifecycleContext::NotifyContextDestroyed() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!was_context_destroyed_);
  was_context_destroyed_ = true;
  LifecycleNotifier::NotifyContextDestroyed();
}

}  // namespace blink
