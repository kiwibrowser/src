// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/worker_animation_frame_provider.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

WorkerAnimationFrameProvider::WorkerAnimationFrameProvider(
    ExecutionContext* context,
    const BeginFrameProviderParams& begin_frame_provider_params)
    : begin_frame_provider_(
          std::make_unique<BeginFrameProvider>(begin_frame_provider_params,
                                               this)),
      callback_collection_(context) {}

int WorkerAnimationFrameProvider::RegisterCallback(
    FrameRequestCallbackCollection::FrameCallback* callback) {
  FrameRequestCallbackCollection::CallbackId id =
      callback_collection_.RegisterCallback(callback);
  begin_frame_provider_->RequestBeginFrame();
  return id;
}

void WorkerAnimationFrameProvider::CancelCallback(int id) {
  callback_collection_.CancelCallback(id);
}

void WorkerAnimationFrameProvider::BeginFrame() {
  double time = WTF::CurrentTimeTicksInMilliseconds();
  // We don't want to expose microseconds residues to users.
  time = round(time * 60) / 60;

  callback_collection_.ExecuteCallbacks(time, time);

  for (auto& ctx : rendering_contexts_) {
    ctx->PushFrame();
  }
}

void WorkerAnimationFrameProvider::AddContextToDispatch(
    CanvasRenderingContext* context) {
  DCHECK(rendering_contexts_.Find(context) == kNotFound);
  rendering_contexts_.push_back(context);
}

void WorkerAnimationFrameProvider::RemoveContextToDispatch(
    CanvasRenderingContext* context) {
  size_t pos = rendering_contexts_.Find(context);
  if (pos != kNotFound) {
    rendering_contexts_.EraseAt(pos);
  }
}

}  // namespace blink
