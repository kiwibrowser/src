// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATION_WORKLET_PROXY_CLIENT_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATION_WORKLET_PROXY_CLIENT_IMPL_H_

#include "base/single_thread_task_runner.h"
#include "third_party/blink/renderer/core/dom/animation_worklet_proxy_client.h"
#include "third_party/blink/renderer/modules/animationworklet/animation_worklet_global_scope.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/graphics/compositor_animator.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class CompositorMutatorImpl;
class Document;
class WorkletGlobalScope;

// Mediates between one Animator and the associated CompositorMutatorImpl. There
// is one AnimationWorkletProxyClientImpl per Animator but there may be multiple
// for a given mutator and animatorWorklet.
//
// This is constructed on the main thread but it is used in the worklet backing
// thread.
class MODULES_EXPORT AnimationWorkletProxyClientImpl final
    : public GarbageCollectedFinalized<AnimationWorkletProxyClientImpl>,
      public AnimationWorkletProxyClient,
      public CompositorAnimator {
  WTF_MAKE_NONCOPYABLE(AnimationWorkletProxyClientImpl);
  USING_GARBAGE_COLLECTED_MIXIN(AnimationWorkletProxyClientImpl);

 public:
  // This client is hooked to the given |mutatee|, on the given
  // |mutatee_runner|.
  explicit AnimationWorkletProxyClientImpl(
      base::WeakPtr<CompositorMutatorImpl> mutatee,
      scoped_refptr<base::SingleThreadTaskRunner> mutatee_runner);
  void Trace(blink::Visitor*) override;

  // AnimationWorkletProxyClient:
  void SetGlobalScope(WorkletGlobalScope*) override;
  void Dispose() override;

  // CompositorAnimator:
  // These methods are invoked on the animation worklet thread.
  std::unique_ptr<CompositorMutatorOutputState> Mutate(
      const CompositorMutatorInputState&) override;

  static AnimationWorkletProxyClientImpl* FromDocument(Document*);

 private:
  base::WeakPtr<CompositorMutatorImpl> mutator_;
  scoped_refptr<base::SingleThreadTaskRunner> mutator_runner_;

  CrossThreadPersistent<AnimationWorkletGlobalScope> global_scope_;

  enum RunState { kUninitialized, kWorking, kDisposed } state_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATION_WORKLET_PROXY_CLIENT_IMPL_H_
