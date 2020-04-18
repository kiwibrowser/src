// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_WORKLET_ANIMATION_CONTROLLER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_WORKLET_ANIMATION_CONTROLLER_H_

#include "third_party/blink/renderer/core/animation/animation_effect.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/heap_allocator.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"

namespace blink {

class Document;
class WorkletAnimationBase;

// Handles AnimationWorklet animations on the main-thread.
//
// The WorkletAnimationController is responsible for owning WorkletAnimation
// instances are long as they are relevant to the animation system. It is also
// responsible for starting valid WorkletAnimations on the compositor side and
// updating WorkletAnimations with updated results from their underpinning
// AnimationWorklet animator instance.
//
// For more details on AnimationWorklet, see the spec:
// https://wicg.github.io/animation-worklet
class CORE_EXPORT WorkletAnimationController
    : public GarbageCollectedFinalized<WorkletAnimationController> {
 public:
  WorkletAnimationController(Document*);
  virtual ~WorkletAnimationController();

  void AttachAnimation(WorkletAnimationBase&);
  void DetachAnimation(WorkletAnimationBase&);
  void InvalidateAnimation(WorkletAnimationBase&);

  void UpdateAnimationCompositingStates();
  void UpdateAnimationTimings(TimingUpdateReason);

  void Trace(blink::Visitor*);

 private:
  HeapHashSet<Member<WorkletAnimationBase>> pending_animations_;
  HeapHashSet<Member<WorkletAnimationBase>> compositor_animations_;

  Member<Document> document_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_WORKLET_ANIMATION_CONTROLLER_H_
