// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_ANIMATION_FRAME_PROVIDER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_ANIMATION_FRAME_PROVIDER_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/frame_request_callback_collection.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context.h"
#include "third_party/blink/renderer/platform/graphics/begin_frame_provider.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class CanvasRenderingContext;

// WorkerAnimationFrameProvider is a member of WorkerGlobalScope and it provides
// RequestAnimationFrame capabilities to Workers.
//
// It's responsible for registering and dealing with callbacks.
// And maintains a connection with the Display process, through
// CompositorFrameSink, that is used to v-sync with the display.
//
// OffscreenCanvases can notify when there's been a change on any
// OffscreenCanvas that is connected to a Canvas, and this class signals
// OffscreenCanvases when it's time to dispatch frames.
class CORE_EXPORT WorkerAnimationFrameProvider
    : public GarbageCollectedFinalized<WorkerAnimationFrameProvider>,
      public BeginFrameProviderClient {
 public:
  static WorkerAnimationFrameProvider* Create(
      ExecutionContext* context,
      const BeginFrameProviderParams& begin_frame_provider_params) {
    return new WorkerAnimationFrameProvider(context,
                                            begin_frame_provider_params);
  }

  int RegisterCallback(FrameRequestCallbackCollection::FrameCallback* callback);
  void CancelCallback(int id);

  void Trace(blink::Visitor* visitor) {
    visitor->Trace(callback_collection_);
    visitor->Trace(rendering_contexts_);
  }

  void TraceWrappers(ScriptWrappableVisitor* visitor) const {
    visitor->TraceWrappers(callback_collection_);
  }

  // BeginFrameProviderClient
  void BeginFrame() override;

  void AddContextToDispatch(CanvasRenderingContext*);
  void RemoveContextToDispatch(CanvasRenderingContext*);

 protected:
  WorkerAnimationFrameProvider(
      ExecutionContext* context,
      const BeginFrameProviderParams& begin_frame_provider_params);

 private:
  const std::unique_ptr<BeginFrameProvider> begin_frame_provider_;
  DISALLOW_COPY_AND_ASSIGN(WorkerAnimationFrameProvider);
  FrameRequestCallbackCollection callback_collection_;

  HeapVector<Member<CanvasRenderingContext>> rendering_contexts_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_ANIMATION_FRAME_PROVIDER_H_
