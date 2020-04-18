// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webaudio/audio_worklet_thread.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"
#include "third_party/blink/renderer/core/workers/worker_backing_thread.h"
#include "third_party/blink/renderer/modules/webaudio/audio_worklet.h"
#include "third_party/blink/renderer/modules/webaudio/audio_worklet_global_scope.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/waitable_event.h"
#include "third_party/blink/renderer/platform/web_thread_supporting_gc.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

template class WorkletThreadHolder<AudioWorkletThread>;

WebThread* AudioWorkletThread::s_backing_thread_ = nullptr;

unsigned AudioWorkletThread::s_ref_count_ = 0;

std::unique_ptr<AudioWorkletThread> AudioWorkletThread::Create(
    ThreadableLoadingContext* loading_context,
    WorkerReportingProxy& worker_reporting_proxy) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("audio-worklet"),
               "AudioWorkletThread::create");
  return base::WrapUnique(
      new AudioWorkletThread(loading_context, worker_reporting_proxy));
}

AudioWorkletThread::AudioWorkletThread(
    ThreadableLoadingContext* loading_context,
    WorkerReportingProxy& worker_reporting_proxy)
    : WorkerThread(loading_context, worker_reporting_proxy) {
  DCHECK(IsMainThread());
  if (++s_ref_count_ == 1)
    EnsureSharedBackingThread();
}

AudioWorkletThread::~AudioWorkletThread() {
  DCHECK(IsMainThread());
  if (--s_ref_count_ == 0)
    ClearSharedBackingThread();
}

WorkerBackingThread& AudioWorkletThread::GetWorkerBackingThread() {
  return *WorkletThreadHolder<AudioWorkletThread>::GetInstance()->GetThread();
}

void CollectAllGarbageOnAudioWorkletThread(WaitableEvent* done_event) {
  blink::ThreadState::Current()->CollectAllGarbage();
  done_event->Signal();
}

void AudioWorkletThread::CollectAllGarbage() {
  DCHECK(IsMainThread());
  WaitableEvent done_event;
  WorkletThreadHolder<AudioWorkletThread>* worklet_thread_holder =
      WorkletThreadHolder<AudioWorkletThread>::GetInstance();
  if (!worklet_thread_holder)
    return;
  worklet_thread_holder->GetThread()->BackingThread().PostTask(
      FROM_HERE, CrossThreadBind(&CollectAllGarbageOnAudioWorkletThread,
                                 CrossThreadUnretained(&done_event)));
  done_event.Wait();
}

void AudioWorkletThread::EnsureSharedBackingThread() {
  DCHECK(IsMainThread());
  if (!s_backing_thread_)
    s_backing_thread_ = Platform::Current()->CreateWebAudioThread().release();
  WorkletThreadHolder<AudioWorkletThread>::EnsureInstance(s_backing_thread_);
}

void AudioWorkletThread::ClearSharedBackingThread() {
  DCHECK(IsMainThread());
  DCHECK(s_backing_thread_);
  DCHECK_EQ(s_ref_count_, 0u);
  WorkletThreadHolder<AudioWorkletThread>::ClearInstance();
  delete s_backing_thread_;
  s_backing_thread_ = nullptr;
}

WebThread* AudioWorkletThread::GetSharedBackingThread() {
  DCHECK(IsMainThread());
  WorkletThreadHolder<AudioWorkletThread>* instance =
      WorkletThreadHolder<AudioWorkletThread>::GetInstance();
  return &(instance->GetThread()->BackingThread().PlatformThread());
}

void AudioWorkletThread::CreateSharedBackingThreadForTest() {
  if (!s_backing_thread_)
    s_backing_thread_ = Platform::Current()->CreateWebAudioThread().release();
  WorkletThreadHolder<AudioWorkletThread>::CreateForTest(s_backing_thread_);
}

WorkerOrWorkletGlobalScope* AudioWorkletThread::CreateWorkerGlobalScope(
    std::unique_ptr<GlobalScopeCreationParams> creation_params) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("audio-worklet"),
               "AudioWorkletThread::createWorkerGlobalScope");
  return AudioWorkletGlobalScope::Create(std::move(creation_params),
                                         GetIsolate(), this);
}

}  // namespace blink
