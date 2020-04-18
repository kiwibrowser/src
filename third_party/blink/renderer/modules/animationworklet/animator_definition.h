// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATOR_DEFINITION_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATOR_DEFINITION_H_

#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_v8_reference.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "v8/include/v8.h"

namespace blink {

// Represents a valid registered Javascript animator.  In particular it owns two
// |v8::Function|s that are the "constructor" and "animate" functions of the
// registered class. It does not do any validation itself and relies on
// |AnimationWorkletGlobalScope::registerAnimator| to validate the provided
// Javascript class before completing the registration.
class MODULES_EXPORT AnimatorDefinition final
    : public GarbageCollectedFinalized<AnimatorDefinition>,
      public TraceWrapperBase {
 public:
  AnimatorDefinition(v8::Isolate*,
                     v8::Local<v8::Function> constructor,
                     v8::Local<v8::Function> animate);
  ~AnimatorDefinition();
  void Trace(blink::Visitor* visitor) {}
  void TraceWrappers(ScriptWrappableVisitor*) const override;
  const char* NameInHeapSnapshot() const override {
    return "AnimatorDefinition";
  }

  v8::Local<v8::Function> ConstructorLocal(v8::Isolate*);
  v8::Local<v8::Function> AnimateLocal(v8::Isolate*);

 private:
  // This object keeps the constructor function, and animate function alive.
  // It participates in wrapper tracing as it holds onto V8 wrappers.
  TraceWrapperV8Reference<v8::Function> constructor_;
  TraceWrapperV8Reference<v8::Function> animate_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_ANIMATIONWORKLET_ANIMATOR_DEFINITION_H_
