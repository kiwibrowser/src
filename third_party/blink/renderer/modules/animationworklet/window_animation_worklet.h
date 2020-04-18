// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_WINDOW_ANIMATION_WORKLET_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_WINDOW_ANIMATION_WORKLET_H_

#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/modules/animationworklet/animation_worklet.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class Document;
class LocalDOMWindow;

class MODULES_EXPORT WindowAnimationWorklet final
    : public GarbageCollected<WindowAnimationWorklet>,
      public Supplement<LocalDOMWindow>,
      public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(WindowAnimationWorklet);

 public:
  static const char kSupplementName[];

  static AnimationWorklet* animationWorklet(LocalDOMWindow&);

  void ContextDestroyed(ExecutionContext*) override;

  void Trace(blink::Visitor*) override;

 private:
  static WindowAnimationWorklet& From(LocalDOMWindow&);

  explicit WindowAnimationWorklet(Document*);

  Member<AnimationWorklet> animation_worklet_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_WINDOW_ANIMATION_WORKLET_H_
