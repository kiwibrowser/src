// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_WORKLET_THREAD_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_WORKLET_THREAD_H_

#include <memory>
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/core/workers/worklet_thread_holder.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

class WebThread;
class WorkerReportingProxy;

// AudioWorkletThread is a per-frame singleton object that represents the
// backing thread for the processing of AudioWorkletNode/AudioWorkletProcessor.
// It is supposed to run an instance of V8 isolate. The life cycle of this
// object is managed by the reference counting of the static backing thread.

class MODULES_EXPORT AudioWorkletThread final : public WorkerThread {
 public:
  static std::unique_ptr<AudioWorkletThread> Create(ThreadableLoadingContext*,
                                                    WorkerReportingProxy&);
  ~AudioWorkletThread() override;

  WorkerBackingThread& GetWorkerBackingThread() override;

  // The backing thread is cleared by clearSharedBackingThread().
  void ClearWorkerBackingThread() override {}

  // This may block the main thread.
  static void CollectAllGarbage();

  static void EnsureSharedBackingThread();
  static void ClearSharedBackingThread();

  static void CreateSharedBackingThreadForTest();

  // This only can be called after EnsureSharedBackingThread() is performed.
  // Currently AudioWorkletThread owns only one thread and it is shared by all
  // the customers.
  static WebThread* GetSharedBackingThread();

 private:
  AudioWorkletThread(ThreadableLoadingContext*, WorkerReportingProxy&);

  WorkerOrWorkletGlobalScope* CreateWorkerGlobalScope(
      std::unique_ptr<GlobalScopeCreationParams>) final;

  bool IsOwningBackingThread() const override { return false; }

  WebThreadType GetThreadType() const override {
    return WebThreadType::kAudioWorkletThread;
  }

  // This raw pointer gets assigned in EnsureSharedBackingThread() and manually
  // released by ClearSharedBackingThread().
  static WebThread* s_backing_thread_;

  // This is only accessed by the main thread. Incremented by the constructor,
  // and decremented by destructor.
  static unsigned s_ref_count_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBAUDIO_AUDIO_WORKLET_THREAD_H_
