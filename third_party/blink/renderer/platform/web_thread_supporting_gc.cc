// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/web_thread_supporting_gc.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/platform/heap/safe_point.h"
#include "third_party/blink/renderer/platform/memory_coordinator.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

namespace blink {

std::unique_ptr<WebThreadSupportingGC> WebThreadSupportingGC::Create(
    const WebThreadCreationParams& params) {
  return base::WrapUnique(new WebThreadSupportingGC(&params, nullptr));
}

std::unique_ptr<WebThreadSupportingGC> WebThreadSupportingGC::CreateForThread(
    WebThread* thread) {
  return base::WrapUnique(new WebThreadSupportingGC(nullptr, thread));
}

WebThreadSupportingGC::WebThreadSupportingGC(
    const WebThreadCreationParams* params,
    WebThread* thread)
    : thread_(thread) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!params || !thread);
#if DCHECK_IS_ON()
  // We call this regardless of whether an existing thread is given or not,
  // as it means that blink is going to run with more than one thread.
  WTF::WillCreateThread();
#endif
  if (!thread_) {
    // If |thread| is not given, create a new one and own it.
    // TODO(scheduler-dev): AnimationWorklet can pass nullptr as WebThread*
    // reference when a test doesn't have a compositor thread.
    owning_thread_ = Platform::Current()->CreateThread(
        params ? *params : WebThreadCreationParams(WebThreadType::kTestThread));
    thread_ = owning_thread_.get();
  }
  MemoryCoordinator::Instance().RegisterThread(thread_);
}

WebThreadSupportingGC::~WebThreadSupportingGC() {
  DETACH_FROM_THREAD(thread_checker_);
  // WebThread's destructor blocks until all the tasks are processed.
  owning_thread_.reset();
  MemoryCoordinator::Instance().UnregisterThread(thread_);
}

void WebThreadSupportingGC::InitializeOnThread() {
  DCHECK(thread_->IsCurrentThread());
  ThreadState::AttachCurrentThread();
  gc_task_runner_ = std::make_unique<GCTaskRunner>(thread_);
}

void WebThreadSupportingGC::ShutdownOnThread() {
  DCHECK(thread_->IsCurrentThread());
#if defined(LEAK_SANITIZER)
  ThreadState::Current()->ReleaseStaticPersistentNodes();
#endif
  // Ensure no posted tasks will run from this point on.
  gc_task_runner_.reset();

  // Shutdown the thread (via its scheduler) only when the thread is created
  // and is owned by this instance.
  if (owning_thread_)
    owning_thread_->Scheduler()->Shutdown();

  ThreadState::DetachCurrentThread();
}

}  // namespace blink
