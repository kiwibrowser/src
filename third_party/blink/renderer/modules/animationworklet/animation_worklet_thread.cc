// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/animationworklet/animation_worklet_thread.h"

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/loader/threadable_loading_context.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"
#include "third_party/blink/renderer/core/workers/worker_backing_thread.h"
#include "third_party/blink/renderer/core/workers/worklet_thread_holder.h"
#include "third_party/blink/renderer/modules/animationworklet/animation_worklet_global_scope.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/web_thread_supporting_gc.h"

namespace blink {

std::unique_ptr<AnimationWorkletThread> AnimationWorkletThread::Create(
    ThreadableLoadingContext* loading_context,
    WorkerReportingProxy& worker_reporting_proxy) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("animation-worklet"),
               "AnimationWorkletThread::create");
  DCHECK(IsMainThread());
  return base::WrapUnique(
      new AnimationWorkletThread(loading_context, worker_reporting_proxy));
}

template class WorkletThreadHolder<AnimationWorkletThread>;

AnimationWorkletThread::AnimationWorkletThread(
    ThreadableLoadingContext* loading_context,
    WorkerReportingProxy& worker_reporting_proxy)
    : WorkerThread(loading_context, worker_reporting_proxy) {}

AnimationWorkletThread::~AnimationWorkletThread() = default;

WebThread* AnimationWorkletThread::GetSharedBackingThread() {
  auto* instance = WorkletThreadHolder<AnimationWorkletThread>::GetInstance();
  DCHECK(instance);
  return &(instance->GetThread()->BackingThread().PlatformThread());
}

WorkerBackingThread& AnimationWorkletThread::GetWorkerBackingThread() {
  return *WorkletThreadHolder<AnimationWorkletThread>::GetInstance()
              ->GetThread();
}

static void CollectAllGarbageOnThread(WaitableEvent* done_event) {
  blink::ThreadState::Current()->CollectAllGarbage();
  done_event->Signal();
}

void AnimationWorkletThread::CollectAllGarbage() {
  DCHECK(IsMainThread());
  WaitableEvent done_event;
  auto* holder = WorkletThreadHolder<AnimationWorkletThread>::GetInstance();
  if (!holder)
    return;
  holder->GetThread()->BackingThread().PostTask(
      FROM_HERE, CrossThreadBind(&CollectAllGarbageOnThread,
                                 CrossThreadUnretained(&done_event)));
  done_event.Wait();
}

void AnimationWorkletThread::EnsureSharedBackingThread() {
  WorkletThreadHolder<AnimationWorkletThread>::EnsureInstance(
      WebThreadCreationParams(WebThreadType::kAnimationWorkletThread));
}

void AnimationWorkletThread::ClearSharedBackingThread() {
  WorkletThreadHolder<AnimationWorkletThread>::ClearInstance();
}

void AnimationWorkletThread::CreateSharedBackingThreadForTest() {
  WorkletThreadHolder<AnimationWorkletThread>::CreateForTest(
      WebThreadCreationParams(WebThreadType::kAnimationWorkletThread));
}

WorkerOrWorkletGlobalScope* AnimationWorkletThread::CreateWorkerGlobalScope(
    std::unique_ptr<GlobalScopeCreationParams> creation_params) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("animation-worklet"),
               "AnimationWorkletThread::CreateWorkerGlobalScope");
  return AnimationWorkletGlobalScope::Create(std::move(creation_params),
                                             GetIsolate(), this);
}

}  // namespace blink
